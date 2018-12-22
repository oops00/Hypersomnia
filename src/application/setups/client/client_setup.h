#pragma once
#include "augs/math/camera_cone.h"
#include "game/detail/render_layer_filter.h"
#include "application/setups/client/client_start_input.h"
#include "application/intercosm.h"
#include "game/cosmos/cosmos.h"
#include "game/cosmos/entity_handle.h"

#include "application/setups/default_setup_settings.h"
#include "application/input/entropy_accumulator.h"

#include "application/setups/setup_common.h"
#include "application/setups/server/server_vars.h"

#include "application/network/network_common.h"

#include "augs/network/network_types.h"
#include "application/setups/server/server_vars.h"

#include "application/predefined_rulesets.h"
#include "application/arena/mode_and_rules.h"
#include "augs/readwrite/memory_stream_declaration.h"
#include "augs/misc/serialization_buffers.h"

#include "view/mode_gui/arena/arena_gui_mixin.h"
#include "application/network/client_state_type.h"
#include "application/network/requested_client_settings.h"

#include "application/network/simulation_receiver.h"
#include "application/session_profiler.h"

struct config_lua_table;

class client_adapter;

enum class client_arena_type {
	PREDICTED,
	REFERENTIAL
};

class client_setup : 
	public default_setup_settings,
	public arena_gui_mixin<client_setup>
{
	using arena_base = arena_gui_mixin<client_setup>;
	friend arena_base;
	friend client_adapter;

	/* This is loaded from the arena folder */
	intercosm scene;
	cosmos_solvable_significant initial_signi;

	predefined_rulesets rulesets;

	/* Other replicated state */
	online_mode_and_rules current_mode;
	server_vars sv_vars;

	mode_player_id client_player_id;

	cosmos predicted_cosmos;
	online_mode_and_rules predicted_mode;

	/* The rest is client-specific */
	sol::state& lua;

	simulation_receiver receiver;

	client_start_input last_start;
	client_state_type state = client_state_type::INVALID;

	client_vars vars;
	requested_client_settings requested_settings;
	bool resend_requested_settings = false;

	entropy_accumulator total_collected;
	augs::serialization_buffers buffers;

	augs::propagate_const<std::unique_ptr<client_adapter>> client;
	net_time_t client_time = 0.0;
	net_time_t when_initiated_connection = 0.0;

	double default_inv_tickrate = 1 / 60.0;

	std::string last_disconnect_reason;

	/* No client state follows later in code. */

	static net_time_t get_current_time();

	template <class H, class S>
	static decltype(auto) get_arena_handle_impl(S& self, const client_arena_type t) {
		if (t == client_arena_type::PREDICTED) {
			return H {
				self.predicted_mode,
				self.scene,
				self.predicted_cosmos,
				self.rulesets,
				self.initial_signi
			};
		}
		else {
			ensure_eq(t, client_arena_type::REFERENTIAL);

			return H {
				self.current_mode,
				self.scene,
				self.scene.world,
				self.rulesets,
				self.initial_signi
			};
		}
	}

	void handle_server_messages();
	void send_pending_commands();
	void send_packets_if_its_time();

	template <class T, class F>
	message_handler_result handle_server_message(
		F&& read_payload
	);

	void send_to_server(total_client_entropy&);

	client_arena_type get_viewed_arena_type() const;

public:
	static constexpr auto loading_strategy = viewables_loading_type::LOAD_ALL;
	static constexpr bool handles_window_input = true;
	static constexpr bool has_additional_highlights = false;

	client_setup(
		sol::state& lua,
		const client_start_input&
	);

	~client_setup();

	void init_connection(const client_start_input&);

	const cosmos& get_viewed_cosmos() const;

	auto get_interpolation_ratio() const {
		const auto dt = get_viewed_cosmos().get_fixed_delta().in_seconds<double>();
		return (get_current_time() - client_time) / dt;
	}

	entity_id get_viewed_character_id() const;

	auto get_viewed_character() const {
		return get_viewed_cosmos()[get_viewed_character_id()];
	}

	const auto& get_viewable_defs() const {
		return scene.viewables;
	}

	custom_imgui_result perform_custom_imgui(perform_custom_imgui_input);

	void customize_for_viewing(config_lua_table&) const;

	void apply(const config_lua_table&);

	double get_audiovisual_speed() const;
	double get_inv_tickrate() const;

	template <class Callbacks>
	void advance(
		const client_advance_input& in,
		const Callbacks& callbacks
	) {
		auto& performance = in.network_performance;

		const auto current_time = get_current_time();

		while (client_time <= current_time) {
			{
				auto scope = measure_scope(performance.receiving_messages);
				handle_server_messages();
			}

			const auto new_local_entropy = [&]() -> std::optional<mode_entropy> {
				if (is_gameplay_on()) {
					return total_collected.extract(
						get_viewed_character(), 
						get_local_player_id(), 
						{ in.settings, in.screen_size }
					);
				}

				return std::nullopt;
			}();

			const bool in_game = new_local_entropy != std::nullopt;

			if (is_connected()) {
				send_pending_commands();

				if (in_game) {
					auto new_client_entropy = new_local_entropy->get_for(
						get_viewed_character(), 
						get_local_player_id()
					);

					send_to_server(new_client_entropy);
				}
			}

			{
				auto scope = measure_scope(performance.sending_messages);
				send_packets_if_its_time();
			}

			if (in_game) {
				{
					auto scope = measure_scope(performance.unpacking_remote_steps);

					auto referential_arena = get_arena_handle(client_arena_type::REFERENTIAL);
					auto predicted_arena = get_arena_handle(client_arena_type::PREDICTED);

					auto advance_referential = [&](const auto& entropy) {
						referential_arena.advance(entropy, solver_callbacks());
					};

					auto advance_repredicted = [&](const auto& entropy) {
						predicted_arena.advance(entropy, solver_callbacks());
					};

					const auto result = receiver.unpack_deterministic_steps(
						in.simulation_receiver,
						in.interp,
						in.past_infection,

						get_viewed_character(),

						referential_arena,
						predicted_arena,

						advance_referential,
						advance_repredicted
					);

					performance.accepted_commands.measure(result.total_accepted);

					if (result.malicious_server) {
						LOG("There was a problem unpacking steps from the server. Disconnecting.");
						log_malicious_server();
						disconnect();
					}
				}

				{
					auto& p = receiver.predicted_entropies;

					const auto& max_commands = vars.max_predicted_client_commands;
					const auto num_commands = p.size();

					if (num_commands > max_commands) {
						last_disconnect_reason = typesafe_sprintf(
							"Number of predicted commands (%x) exceeded max_predicted_client_commands (%x).", 
							num_commands,
							max_commands
						);

						disconnect();
					}

					performance.predicted_steps.measure(num_commands);

					p.push_back(*new_local_entropy);
				}

#if USE_CLIENT_PREDICTION
				//LOG("PE: %x", receiver.predicted_entropies.size());
				get_arena_handle(client_arena_type::PREDICTED).advance(*new_local_entropy, callbacks);
#else
				(void)callbacks;
#endif
			}

			if (in_game) {
				client_time += get_inv_tickrate();
			}
			else {
				client_time += default_inv_tickrate;
			}

			update_stats(in.network_stats);
			total_collected.clear();
		}
	}

	template <class T>
	void control(const T& t) {
		total_collected.control(t);
	}

	void accept_game_gui_events(const game_gui_entropy_type&);

	augs::path_type get_unofficial_content_dir() const;

	auto get_render_layer_filter() const {
		return render_layer_filter::disabled();
	}

	void ensure_handler() {}

	mode_player_id get_local_player_id() const {
		return client_player_id;
	}

	online_arena_handle<false> get_arena_handle(std::optional<client_arena_type> = std::nullopt);
	online_arena_handle<true> get_arena_handle(std::optional<client_arena_type> = std::nullopt) const;

	void log_malicious_server();

	bool is_connected() const;
	void disconnect();

	bool is_gameplay_on() const;
	setup_escape_result escape();

	void update_stats(network_info&) const;
};
