/** @file mcf.cpp Definition of Multi-Commodity-Flow solver */

#include "mcf.h"
#include "../core/math_func.hpp"

MultiCommodityFlow::MultiCommodityFlow() :
	graph(NULL)
{}

void MultiCommodityFlow::Run(LinkGraphComponent * g) {
	assert(g->GetSettings().mcf_accuracy >= 1);
	graph = g;
}

bool DistanceAnnotation::IsBetter(const DistanceAnnotation * base, int cap, uint dist) const {
	if (cap > 0 && base->capacity > 0) {
		if (capacity <= 0) {
			return true; // if the other path has capacity left and this one hasn't, the other one's better
		} else {
			return base->distance + dist < distance;
		}
	} else {
		return false; // if the other path doesn't have capacity left, this one is always better
	}
}

bool CapacityAnnotation::IsBetter(const CapacityAnnotation * base, int cap, uint dist) const {
	int min_cap = min(base->capacity, cap);
	if (min_cap == capacity) { // if the capacities are the same, choose the shorter path
		return (base->distance + dist < distance);
	} else {
		return min_cap > capacity;
	}
}


template<class ANNOTATION>
void MultiCommodityFlow::Dijkstra(NodeID from, PathVector & paths, uint max_hops, bool create_new_paths) {
	typedef std::set<ANNOTATION *, typename ANNOTATION::comp> AnnoSet;
	uint size = graph->GetSize();
	AnnoSet annos;
	paths.resize(size, NULL);
	for (NodeID node = 0; node < size; ++node) {
		ANNOTATION * anno = new ANNOTATION(node, node == from);
		annos.insert(anno);
		paths[node] = anno;
	}
	while(!annos.empty()) {
		typename AnnoSet::iterator i = annos.begin();
		ANNOTATION * source = *i;
		annos.erase(i);
		if(source->GetHops() == max_hops) continue;
		NodeID from = source->GetNode();
		NodeID to = graph->GetFirstEdge(from);
		while (to != Node::INVALID) {
			Edge & edge = graph->GetEdge(from, to);
			if (edge.capacity > 0 && (create_new_paths || edge.flow > 0)) {
				int capacity = edge.capacity - edge.flow;
				uint distance = edge.distance;
				ANNOTATION * dest = static_cast<ANNOTATION *>(paths[to]);
				if (dest->IsBetter(source, capacity, distance)) {
					annos.erase(dest);
					dest->Fork(source, capacity, distance);
					annos.insert(dest);
				}
			}
			to = edge.next_edge;
		}
	}
}



void MultiCommodityFlow::CleanupPaths(PathVector & paths) {
	for(PathVector::iterator i = paths.begin(); i != paths.end(); ++i) {
		Path * path = *i;
		while (path != NULL && path->GetFlow() == 0) {
			Path * parent = path->GetParent();
			path->UnFork();
			if (path->GetNumChildren() == 0) {
				NodeID node = path->GetNode();
				delete path;
				paths[node] = NULL;
			}
			path = parent;
		}
	}
	paths.clear();
}

void MultiCommodityFlow::PushFlow(Edge & edge, Path * path, uint accuracy, bool positive_cap) {
	uint flow = edge.unsatisfied_demand / accuracy;
	if (flow == 0) flow = 1;
	flow = path->AddFlow(flow, graph, positive_cap);
	if (flow > 0) {
		edge.unsatisfied_demand -= flow;
	}
}


void MCF1stPass::Run(LinkGraphComponent * graph) {
	MultiCommodityFlow::Run(graph);
	PathVector paths;
	uint size = graph->GetSize();
	uint accuracy = graph->GetSettings().mcf_accuracy;
	bool demand_left = true;
	bool decrease_accuracy = true;
	uint hops = 0;
	while (demand_left && hops < size) {
		demand_left = false;
		if (decrease_accuracy) {
			accuracy = graph->GetSettings().mcf_accuracy;
			uint tmp = Power(size, hops);
			if (tmp < accuracy) {
				accuracy = tmp;
			} else {
				decrease_accuracy = false;
			}
		}
		hops++;
		for (NodeID source = 0; source < size; ++source) {
			/* first saturate the shortest paths */
			Dijkstra<DistanceAnnotation>(source, paths, hops, true);

			for (NodeID dest = 0; dest < size; ++dest) {
				Edge & edge = graph->GetEdge(source, dest);
				if (edge.unsatisfied_demand > 0) {
					Path * path = paths[dest];
					if (path->GetCapacity() > 0) {
						PushFlow(edge, path, accuracy, true);
					}
					if (edge.unsatisfied_demand > 0) {
						demand_left = true;
					}
				}
			}
			CleanupPaths(paths);
		}
		if (accuracy > 1) --accuracy;
	}
}

void MCF2ndPass::Run(LinkGraphComponent * graph) {
	MultiCommodityFlow::Run(graph);
	PathVector paths;
	uint size = graph->GetSize();
	uint accuracy = graph->GetSettings().mcf_accuracy;
	bool demand_left = true;
	while (demand_left) {
		demand_left = false;
		for (NodeID source = 0; source < size; ++source) {
			/* first saturate the shortest paths */
			Dijkstra<CapacityAnnotation>(source, paths, size, false);
			for (NodeID dest = 0; dest < size; ++dest) {
				Edge & edge = graph->GetEdge(source, dest);
				if (edge.unsatisfied_demand > 0) {
					Path * path = paths[dest];
					PushFlow(edge, path, accuracy, false);
					if (edge.unsatisfied_demand > 0) {
						demand_left = true;
					}
				}
			}
			CleanupPaths(paths);
		}
		if (accuracy > 1) --accuracy;
	}
}

/**
 * avoid accidentally deleting different paths of the same capacity/distance in a set.
 * When the annotation is the same the pointers themselves are compared, so there are no equal ranges.
 * (The problem might have been something else ... but this isn't expensive I guess)
 */
bool greater(uint x_anno, uint y_anno, const Path * x, const Path * y) {
	if (x_anno > y_anno) {
		return true;
	} else if (x_anno < y_anno) {
		return false;
	} else {
		return x > y;
	}
}

bool CapacityAnnotation::comp::operator()(const CapacityAnnotation * x, const CapacityAnnotation * y) const {
	return greater(x->GetAnnotation(), y->GetAnnotation(), x, y);
}

bool DistanceAnnotation::comp::operator()(const DistanceAnnotation * x, const DistanceAnnotation * y) const {
	return x != y && !greater(x->GetAnnotation(), y->GetAnnotation(), x, y);
}
