#include "stdafx.h"
#include "visibility_system.h"

#include "entity_system/world.h"
#include "entity_system/entity.h"

#include "physics_system.h"
#include "render_system.h"

#include "../resources/render_info.h"
#include "../game/body_helper.h"

#include <limits>
#include <set>

struct my_callback : public b2QueryCallback {
	std::set<b2Body*> bodies;
	entity* subject;
	b2Filter* filter;

	my_callback() : subject(nullptr), filter(nullptr) {}

	bool ReportFixture(b2Fixture* fixture) override {
		if ((b2ContactFilter::ShouldCollide(filter, &fixture->GetFilterData()))
			&&
			(entity*) fixture->GetBody()->GetUserData() != subject)
			bodies.insert(fixture->GetBody());
		return true;
	}
};

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

/*
source:
http://stackoverflow.com/questions/16542042/fastest-way-to-sort-vectors-by-angle-without-actually-computing-that-angle
*/
float comparable_angle(vec2<> diff) {
	return sgn(diff.y) * (
		1 - (diff.x / (std::abs(diff.x) + std::abs(diff.y)))
		);
}

visibility_system::visibility_system() : draw_cast_rays(false), draw_triangle_edges(true), draw_discontinuities(false), draw_visible_walls(false) {}

int components::visibility::layer::get_num_triangles() {
	return edges.size();
}

components::visibility::discontinuity* components::visibility::layer::get_discontinuity(int n) {
	for (auto& disc : discontinuities)
		if (disc.edge_index == n)
			return &disc;

	return nullptr;
}

components::visibility::triangle components::visibility::layer::get_triangle(int i, augmentations::vec2<> origin) {
	components::visibility::triangle tri = { origin, edges[i].first, edges[i].second };
	return tri;
}

void visibility_system::process_entities(world& owner) {
	/* prepare epsilons to be used later, just to make the notation more clear */
	float epsilon_distance_vertex_hit_sq = epsilon_distance_vertex_hit * PIXELS_TO_METERSf;
	float epsilon_threshold_obstacle_hit_sq = epsilon_threshold_obstacle_hit * PIXELS_TO_METERSf;
	epsilon_distance_vertex_hit_sq *= epsilon_distance_vertex_hit_sq;
	epsilon_threshold_obstacle_hit_sq *= epsilon_threshold_obstacle_hit_sq;

	/* we'll need a reference to physics system for raycasting */
	physics_system& physics = owner.get_system<physics_system>();
	/* we'll need a reference to render system for debug drawing */
	render_system& render = owner.get_system<render_system>();

	for (auto it : targets) {
		/* get AI data and position of the entity */
		auto& visibility = it->get<components::visibility>();
		auto& transform = it->get<components::transform>().current;

		auto body = it->get<components::physics>().body;

		/* transform entity position to Box2D coordinates */
		vec2<> position_meters = transform.pos * PIXELS_TO_METERSf;

		/* for every visibility type requested for given entity */
		for (auto& entry : visibility.visibility_layers.raw) {
			/* prepare container for all the vertices that we will cast the ray to */
			static std::vector < std::pair < float, vec2< >> > all_vertices_transformed;
			all_vertices_transformed.clear();

			/* shortcut */
			auto& request = entry.val;
			/* to Box2D coordinates */
			float vision_side_meters = request.square_side * PIXELS_TO_METERSf;

			/* prepare maximum visibility square */
			b2AABB aabb;
			aabb.lowerBound = position_meters - vision_side_meters / 2;
			aabb.upperBound = position_meters + vision_side_meters / 2;

			/* get list of all fixtures that intersect with the visibility square */
			my_callback callback;
			callback.subject = it;
			callback.filter = &request.filter;
			physics.b2world.QueryAABB(&callback, aabb);

			/* for every fixture that intersected with the visibility square */
			for (auto b : callback.bodies) {
				auto verts = topdown::get_transformed_shape_verts(*reinterpret_cast<entity*>(b->GetUserData()));
				/* for every vertex in given fixture's shape */
				for (auto& v : verts) {
					std::pair<float, vec2<>> new_vertex;
					/* transform vertex to current entity's position and rotation */
					new_vertex.second = v;

					vec2<> diff = new_vertex.second - position_meters;
					/*
					compute angle to be compared while sorting
					source:
					http://stackoverflow.com/questions/16542042/fastest-way-to-sort-vectors-by-angle-without-actually-computing-that-angle

					save the angle in pair next to the vertex position, we will then sort the "angle-vertex" pairs by angle */
					new_vertex.first = comparable_angle(diff);

					/* save transformed vertex */
					all_vertices_transformed.push_back(new_vertex);
				}
			}

			/* extract the actual vertices from visibility AABB to cast rays to */
			b2Vec2 whole_vision [] = {
				aabb.lowerBound,
				aabb.lowerBound + vec2<>(vision_side_meters, 0),
				aabb.upperBound,
				aabb.upperBound - vec2<>(vision_side_meters, 0)
			};

			/* prepare edge shapes given above vertices to cast rays against when no obstacle was hit
			note we lengthen them a bit and add/substract 1.f to avoid undeterministic vertex cases
			*/
			b2EdgeShape bounds[4];
			bounds[0].Set(vec2<>(whole_vision[0]) + vec2<>(-1.f, 0.f), vec2<>(whole_vision[1]) + vec2<>(1.f, 0.f));
			bounds[1].Set(vec2<>(whole_vision[1]) + vec2<>(0.f, -1.f), vec2<>(whole_vision[2]) + vec2<>(0.f, 1.f));
			bounds[2].Set(vec2<>(whole_vision[2]) + vec2<>(1.f, 0.f), vec2<>(whole_vision[3]) + vec2<>(-1.f, 0.f));
			bounds[3].Set(vec2<>(whole_vision[3]) + vec2<>(0.f, 1.f), vec2<>(whole_vision[0]) + vec2<>(0.f, -1.f));

			/* debug drawing of the visibility square */
			if (draw_cast_rays || draw_triangle_edges) {
				render.lines.push_back(render_system::debug_line((vec2<>(whole_vision[0]) + vec2<>(-1.f, 0.f))*METERS_TO_PIXELSf, (vec2<>(whole_vision[1]) + vec2<>(1.f, 0.f))*METERS_TO_PIXELSf));
				render.lines.push_back(render_system::debug_line((vec2<>(whole_vision[1]) + vec2<>(0.f, -1.f))*METERS_TO_PIXELSf, (vec2<>(whole_vision[2]) + vec2<>(0.f, 1.f))*METERS_TO_PIXELSf));
				render.lines.push_back(render_system::debug_line((vec2<>(whole_vision[2]) + vec2<>(1.f, 0.f))*METERS_TO_PIXELSf, (vec2<>(whole_vision[3]) + vec2<>(-1.f, 0.f))*METERS_TO_PIXELSf));
				render.lines.push_back(render_system::debug_line((vec2<>(whole_vision[3]) + vec2<>(0.f, 1.f))*METERS_TO_PIXELSf, (vec2<>(whole_vision[0]) + vec2<>(0.f, -1.f))*METERS_TO_PIXELSf));
			}

			/* add the visibility square to the vertices that we cast rays to, computing comparable angle in place */
			for (auto& v : whole_vision)
				all_vertices_transformed.push_back(std::make_pair(comparable_angle(v - position_meters), v));

			/* SORT ALL VERTICES BY ANGLE */
			std::sort(all_vertices_transformed.begin(), all_vertices_transformed.end());

			/* by now we have ensured that all_vertices_transformed is non-empty

			debugging:
			red ray - ray that intersected with obstacle, these are ignored
			yellow ray - ray that hit the same vertex
			violet ray - ray that passed through vertex and hit another obstacle
			blue ray - ray that passed through vertex and hit boundary
			*/

			/* double_ray pair for holding both left-epsilon and right-epsilon rays */
			struct double_ray {
				vec2<> first, second;
				bool first_reached_destination, second_reached_destination;

				double_ray() : first_reached_destination(false), second_reached_destination(false) {}
				double_ray(vec2<> first, vec2<> second, bool a, bool b)
					: first(first), second(second), first_reached_destination(a), second_reached_destination(b) {
				}
			};
			std::vector<double_ray> double_rays;

			/* debugging lambda */
			auto draw_line = [&position_meters, &render](vec2<> point, graphics::pixel_32 col){
				render.lines.push_back(render_system::debug_line(position_meters * METERS_TO_PIXELSf, point * METERS_TO_PIXELSf, col));
			};

			/* container for holding info about local discontinuities, will be used for dynamic AI navigation */
			request.discontinuities.clear();

			request.vertex_hits.clear();

			for (auto& vertex : all_vertices_transformed) {
				b2Vec2* from_aabb = nullptr;

				for (auto& aabb_vert : whole_vision)
					if (vertex.second == aabb_vert)
						from_aabb = &aabb_vert;

				/* save the entity pointer in ray_callback structure to not intersect with the player fixture
				it is then compared in my_ray_callback::ReportFixture
				*/
				
				/* create a vector in direction of vertex with length equal to the half of diagonal of the visibility square
				(majority of rays will slightly SLIGHTLY go beyond visibility square, but that's not important for now)
				ray_callbacks[0] and ray_callbacks[1] differ ONLY by an epsilon added / substracted to the angle
				*/

				physics_system::raycast_output ray_callbacks[2];
				vec2<> targets[2] = {
					position_meters + vec2<>::from_degrees((vertex.second - position_meters).get_degrees() - epsilon_ray_angle_variation) * vision_side_meters / 2 * 1.414213562373095,
					position_meters + vec2<>::from_degrees((vertex.second - position_meters).get_degrees() + epsilon_ray_angle_variation) * vision_side_meters / 2 * 1.414213562373095
				};
				
				/* cast both rays starting from the player position and ending in ray_callbacks[x].target, ignoring subject entity completely */
				ray_callbacks[0] = physics.ray_cast(position_meters, targets[0], &request.filter, it);
				ray_callbacks[1] = physics.ray_cast(position_meters, targets[1], &request.filter, it);

				/* if we did not intersect with anything */
				if (!(ray_callbacks[0].hit || ray_callbacks[1].hit)) {
					/* we must have cast the ray against AABB */
					if (from_aabb) {
						double_rays.push_back(double_ray(*from_aabb, *from_aabb, true, true));
					}
					/* only ever happens if an object is only partially inside visibility rectangle
					may happen but ignore, handling not implemented
					*/
					else {
						bool breakpoint = true;
					}
				}
				/* both rays intersect with something */
				else if (ray_callbacks[0].hit && ray_callbacks[1].hit) {
					/* if distance between intersection and target vertex is bigger than epsilon, which means that
					BOTH intersections occured FAR from the vertex
					then ray must have intersected with an obstacle BEFORE reaching the vertex, ignoring intersection completely */
					float distance_from_origin = (vertex.second - position_meters).length_sq();

					if ((ray_callbacks[0].intersection - position_meters).length_sq() + epsilon_threshold_obstacle_hit_sq < distance_from_origin &&
						(ray_callbacks[1].intersection - position_meters).length_sq() + epsilon_threshold_obstacle_hit_sq < distance_from_origin) {
							if (draw_cast_rays) draw_line(ray_callbacks[0].intersection, graphics::pixel_32(255, 0, 0, 255));
					}
					/* distance between both intersections fit in epsilon which means ray intersected with the same vertex */
					else if ((ray_callbacks[0].intersection - ray_callbacks[1].intersection).length_sq() < epsilon_distance_vertex_hit_sq) {
						/* interpret it as both rays hit the same vertex
						for maximum accuracy, push the vertex coordinates instead of the actual intersections */
						request.vertex_hits.push_back(vertex.second * METERS_TO_PIXELSf);
						double_rays.push_back(double_ray(vertex.second, vertex.second, true, true));
						if (draw_cast_rays) draw_line(vertex.second, graphics::pixel_32(255, 255, 0, 255));
					}
					/* this is the case where the ray is cast at the lateral vertex,
					here we also detect the discontinuity */
					else {
						/* save both intersection points, this is what we introduced "double_ray" pair for */
						double_ray new_double_ray(ray_callbacks[0].intersection, ray_callbacks[1].intersection, false, false);

						/* handle new discontinuity */
						components::visibility::discontinuity new_discontinuity;

						/* if the ray that we substracted the epsilon from intersected closer (and thus with the vertex), then the free space is to the right */
						if ((ray_callbacks[0].intersection - vertex.second).length_sq() < (ray_callbacks[1].intersection - vertex.second).length_sq()) {
							/* it was "first" one that directly reached its destination */
							new_double_ray.first_reached_destination = true;
							new_double_ray.first = vertex.second;
							
							new_discontinuity.points.first = vertex.second;
							new_discontinuity.points.second = ray_callbacks[1].intersection;
							new_discontinuity.winding = components::visibility::discontinuity::RIGHT;
							new_discontinuity.edge_index = double_rays.size() - 1;
							if (draw_cast_rays) draw_line(ray_callbacks[1].intersection, graphics::pixel_32(255, 0, 255, 255));
						}
						/* otherwise it is to the left */
						else {
							/* it was "second" one that directly reached its destination */
							new_double_ray.second_reached_destination = true;
							new_double_ray.second = vertex.second;

							new_discontinuity.points.first = vertex.second;
							new_discontinuity.points.second = ray_callbacks[0].intersection;
							new_discontinuity.winding = components::visibility::discontinuity::LEFT;
							new_discontinuity.edge_index = double_rays.size();
							if (draw_cast_rays) draw_line(ray_callbacks[0].intersection, graphics::pixel_32(255, 0, 255, 255));
						}
						/* save new double ray */
						double_rays.push_back(new_double_ray);
						/* save new discontinuity */
						request.discontinuities.push_back(new_discontinuity);
					}
				}
				/* the case where exactly one of the rays did not hit anything so we cast it against boundaries,
				we also detect discontinuity here */
				else {
					/* for every callback that didn't detect hit (there will be only one) */
					for (int i = 0; i < 2; ++i) {
						if (!ray_callbacks[i].hit) {
							/* for every edge from 4 edges forming visibility square */
							for (auto& bound : bounds) {
								/* prepare b2RayCastOutput/b2RayCastInput data for raw b2EdgeShape::RayCast call */
								b2RayCastOutput output;
								b2RayCastInput input;
								input.maxFraction = 1.0;
								input.p1 = position_meters;
								input.p2 = targets[i];

								/* we don't need to transform edge or ray since they are in the same space
								but we have to prepare dummy b2Transform as argument for b2EdgeShape::RayCast
								*/
								b2Transform null_transform(b2Vec2(0.f, 0.f), b2Rot(0.f));

								/* if we hit against boundaries (must happen for at least 1 of them) */
								if (bound.RayCast(&output, input, null_transform, 0)) {
									/* prepare new discontinuity data */
									components::visibility::discontinuity new_discontinuity;

									/* compute the actual intersection point from b2RayCastOutput data */
									auto actual_intersection = input.p1 + output.fraction * (input.p2 - input.p1);

									/* if the left-handed ray intersected with boundary */
									if (i == 0) {
										new_discontinuity.points.first = ray_callbacks[1].intersection;
										new_discontinuity.points.second = ray_callbacks[0].intersection;
										new_discontinuity.winding = components::visibility::discontinuity::LEFT;
										new_discontinuity.edge_index = double_rays.size();
										double_rays.push_back(double_ray(actual_intersection, ray_callbacks[1].intersection, false, true));
									}
									/* if the right-handed ray intersected with boundary */
									else if (i == 1) {
										new_discontinuity.points.first = ray_callbacks[0].intersection;
										new_discontinuity.points.second = ray_callbacks[1].intersection;
										new_discontinuity.winding = components::visibility::discontinuity::RIGHT;
										new_discontinuity.edge_index = double_rays.size() - 1;
										double_rays.push_back(double_ray(ray_callbacks[0].intersection, actual_intersection, true, false));
									}
									request.discontinuities.push_back(new_discontinuity);

									if (draw_cast_rays) draw_line(actual_intersection, graphics::pixel_32(0, 0, 255, 255));
								}
							}
							break;
						}
					}
				}
			}

			/* transform all discontinuities from Box2D coordinates to pixels */
			for (auto& disc : request.discontinuities) {
				disc.points.first *= METERS_TO_PIXELSf;
				disc.points.second *= METERS_TO_PIXELSf;

				/* wrap it */
				if (disc.edge_index < 0) 
					disc.edge_index = double_rays.size() - 1;
			}

			/* now to propagate the output */
			request.edges.clear();

			for (size_t i = 0; i < double_rays.size(); ++i) {
				/* (i + 1)%double_rays.size() ensures the cycle */
				auto& ray_a = double_rays[i];
				auto& ray_b = double_rays[(i + 1)%double_rays.size()];

				vec2<> p1 = ray_a.second * METERS_TO_PIXELSf;
				vec2<> p2 = ray_b.first * METERS_TO_PIXELSf;

				if (draw_triangle_edges) {
					draw_line(p1 * PIXELS_TO_METERSf, request.color);
					draw_line(p2 * PIXELS_TO_METERSf, request.color);
					render.lines.push_back(render_system::debug_line(p1, p2, request.color));
				}

				request.edges.push_back(std::make_pair(p1, p2));
			}

			/* values less than zero indicate we don't want to ignore discontinuities */
			if (request.ignore_discontinuities_shorter_than > 0.f) {
				int edges_num = request.edges.size();

				/* prepare helpful lambda */
				auto& wrap = [edges_num](int ix){
					if (ix < 0) return edges_num + ix;
					return ix % edges_num;
				};

				/* shortcut, note we get it by copy */
				auto discs_copy = request.discontinuities;

				std::vector<components::visibility::edge> marked_holes;

				/* for every discontinuity, remove if there exists an edge that is too close to the discontinuity's vertex */
				discs_copy.erase(std::remove_if(discs_copy.begin(), discs_copy.end(),
					[&request, edges_num, &transform, &wrap, &render, &marked_holes, this]
				(const components::visibility::discontinuity& d){
						std::vector<vec2<>> points_too_close;

						int cw = d.winding == d.RIGHT ? 1 : -1;
						
						/* we check all vertices of edges */
						for (int j = wrap(d.edge_index + cw), k = 0; k < edges_num-1; j = wrap(j + cw), ++k) {
							/* we've already reached edges that are to CW/CCW side of discontinuity, we're not interested in checking further */
							if (cw * (d.points.first - transform.pos).cross(request.edges[j].first - transform.pos) <= 0)
								break;

							/* project this point onto candidate edge */
							vec2<> close_point = d.points.first.closest_point_on_segment(request.edges[j].first, request.edges[j].second);

							if (close_point.compare(d.points.first, request.ignore_discontinuities_shorter_than)) 
								points_too_close.push_back(close_point);
						}
						
						/* let's also check the discontinuities - we don't know what's behind */
						for (auto& old_disc : request.discontinuities) {
							if (old_disc.edge_index != d.edge_index) {
								/* if a discontinuity is to CW/CCW respectively */
								if (!(cw * (d.points.first - transform.pos).cross(old_disc.points.first - transform.pos) <= 0)) {
									/* project this point onto candidate discontinuity */
									vec2<> close_point = d.points.first.closest_point_on_segment(old_disc.points.first, old_disc.points.second);

									if (close_point.compare(d.points.first, request.ignore_discontinuities_shorter_than))
										points_too_close.push_back(close_point);
								}
							}
						}

						if (!points_too_close.empty()) {
							vec2<> closest_point = *std::min_element(points_too_close.begin(), points_too_close.end(), [&transform](vec2<> a, vec2<> b){
								return (a - transform.pos).length_sq() < (b - transform.pos).length_sq();
							});

							marked_holes.push_back(components::visibility::edge(closest_point, d.points.first));
							
							if (draw_discontinuities)
								render.lines.push_back(render_system::debug_line(closest_point, d.points.first, graphics::pixel_32(255, 255, 255, 255)));
							
							return true;
						}

						return false;
				}
				), discs_copy.end());

				/* now check if any remaining discontinuities can not be seen through marked non-walkable areas (pathological case) */
				b2RayCastOutput output;
				b2RayCastInput input;
				input.maxFraction = 1.0;

				for (auto& marked : marked_holes) {
					b2EdgeShape marked_hole;
					marked_hole.Set(marked.first, marked.second);

					discs_copy.erase(std::remove_if(discs_copy.begin(), discs_copy.end(), [&marked_hole, &output, &input, &transform](const components::visibility::discontinuity& d){
						input.p1 = transform.pos;
						input.p2 = d.points.first;

						/* we don't need to transform edge or ray since they are in the same space
						but we have to prepare dummy b2Transform as argument for b2EdgeShape::RayCast
						*/
						b2Transform null_transform(b2Vec2(0.f, 0.f), b2Rot(0.f));

						return (marked_hole.RayCast(&output, input, null_transform, 0));
					}), discs_copy.end());
				}

				/* save cleaned copy in actual discontinuities */
				request.discontinuities = discs_copy;
			}

			if (draw_discontinuities)
				for (auto& disc : request.discontinuities)
					render.lines.push_back(render_system::debug_line(disc.points.first, disc.points.second, graphics::pixel_32(0, 127, 255, 255)));
		}
	}
}