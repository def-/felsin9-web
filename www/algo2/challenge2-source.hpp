/*
 * FlowImplementation.hpp
 */

#ifndef FLOW_IMPLEMENTATION_HPP_
#define FLOW_IMPLEMENTATION_HPP_

//#define DEBUG

#include <vector>
#include <algorithm>
#include <iostream>
#include <stdint.h>

#include "Parser.hpp"

#define UPDATE_FREQUENCY 0.5
#define N_MULTIPLE 6
#define CONSTANT_WORK 12

using namespace std;

struct Edge;

struct Node {
  Edge* firstEdge;
  Edge* currentEdge;

  unsigned int distance;
  unsigned int excess;

  Node* next; // next in bucket
  Node* prev; // previous in bucket
};

struct Edge {
  unsigned int capacity; // residual capacity, not constant
  Node* target;
  Edge* reverse;
};

struct Bucket { // Buckets of nodes by distance, decides which to handle next
  Node* active; // excess > 0, singly-linked list, removal only from front
  Node* inactive; // excess = 0, double-linked list, removal anywhere
};

class FlowGraph{
  private:
    FlowGraph( const FlowGraph & ){} //do not copy
    void operator=( const FlowGraph & ){} //really, do not copy

    static bool comp(Parser::Arc a1, Parser::Arc a2) {
      if (a1.source < a2.source)
        return true;

      if (a1.source > a2.source)
        return false;

      if (a1.target < a2.target)
        return true;

      return false;
    }

    static bool comp2(Parser::Arc a1, Parser::Arc a2) {
      if (a1.target < a2.target)
        return true;

      if (a1.target > a2.target)
        return false;

      if (a1.source < a2.source)
        return true;

      return false;
    }

  public:
    Node* nodes; // begins at 1
    Node* sentinel; // end of v
    Edge* edges; // edges of residual graph

    unsigned int n;
    unsigned int m;

    FlowGraph() {
    }

    // Ugly and inperformant initialization, luckily time doesn't count here
    FlowGraph( const Parser & parser ) {
      n = parser.num_nodes;
      m = parser.arcs.size();

      nodes = (Node*) malloc((n+2) * sizeof(Node));
      edges = (Edge*) malloc((2*m+1) * sizeof(Edge));

      sentinel = &nodes[n+1];

      vector<Parser::Arc> sortedArcs = parser.arcs;
      sort(sortedArcs.begin(), sortedArcs.end(), comp);

      vector<Parser::Arc> rSortedArcs = parser.arcs;
      sort(rSortedArcs.begin(), rSortedArcs.end(), comp2);

      unsigned int e = 0;

      unsigned int si = 0;
      unsigned int ri = 0;

      for (unsigned int v = 1; v <= n; v++) {
        nodes[v].firstEdge = &edges[e];
        nodes[v].currentEdge = &edges[e];

        while (si < sortedArcs.size() && v == sortedArcs.at(si).source) {
          si++;
          e++;
        }

        while (ri < rSortedArcs.size() && v == rSortedArcs.at(ri).target) {
          Parser::Arc rArc = {rSortedArcs.at(ri).target, rSortedArcs.at(ri).source, 0};
          ri++;
          if (!binary_search(sortedArcs.begin(), sortedArcs.end(), rArc, comp)) {
            e++;
          }
        }
      }

      sentinel->firstEdge = &edges[e]; // end of e
      si = 0;

      while (si < sortedArcs.size()) {
        unsigned int v = sortedArcs.at(si).source;
        unsigned int u = sortedArcs.at(si).target;

        Edge* vu = NULL;
        Edge* uv = NULL;

        for (vu = nodes[v].firstEdge; vu < nodes[v].currentEdge; vu++) {
          if (vu->target == &nodes[u]) {
            vu->capacity = sortedArcs.at(si).capacity;

            uv = vu->reverse; // is already initialized

            break;
          }
        }

        if (!uv) {
          vu = (nodes[v].currentEdge)++;
          uv = (nodes[u].currentEdge)++;

          vu->capacity = sortedArcs.at(si).capacity;
          vu->target = &nodes[u];
          vu->reverse = uv;

          uv->capacity = 0;
          uv->target = &nodes[v];
          uv->reverse = vu;
        }

        si++;
      }

      for (unsigned int v = 1; v <= n; v++) {
        nodes[v].currentEdge = nodes[v].firstEdge;
        nodes[v].excess = 0;
      }
    }
};

class FlowImplementation{
private:
  FlowImplementation( const FlowImplementation& ){} //do not copy
  void operator=( const FlowImplementation& ){} //really, do not copy

  unsigned int updateLimit;

  FlowGraph graph;
  Bucket* buckets;

  Node* sink;
  Node* source;

  unsigned int flow;
  unsigned int maxLabel;
  unsigned int minActive;
  unsigned int maxActive;
  unsigned int workCounter;

  void activeAdd(Bucket* b, Node* v)
  {
#ifdef DEBUG/*{{{*/
    cout << "activeAdd(" << b - buckets << ", " << v - graph.nodes << ")" << endl;
#endif/*}}}*/
    v->next = b->active;
    b->active = v;

    if (v->distance < minActive) {
      minActive = v->distance;
    }

    if (v->distance > maxActive) {
      maxActive = v->distance;
    }

    if (maxLabel < maxActive) {
      maxLabel = maxActive;
    }
  }

  void inactiveAdd(Bucket* b, Node* v)
  {
#ifdef DEBUG/*{{{*/
    cout << "inactiveAdd(" << b - buckets << ", " << v - graph.nodes << ")" << endl;
#endif/*}}}*/
    v->next = b->inactive;
    v->prev = graph.sentinel;
    v->next->prev = v;
    b->inactive = v;
  }

  void activeRemove(Bucket* b)
  {
#ifdef DEBUG/*{{{*/
    cout << "activeRemove(" << b - buckets << ") = " << b->active - graph.nodes << endl;
#endif/*}}}*/
    b->active = b->active->next;
  }

  void inactiveRemove(Bucket* b, Node* v)
  {
#ifdef DEBUG/*{{{*/
    cout << "inactiveRemove(" << b - buckets << ", " << v - graph.nodes << ")" << endl;
#endif/*}}}*/
    if (v != b->inactive) {
      v->prev->next = v->next;
      v->next->prev = v->prev;
    } else {
      v->next->prev = graph.sentinel;
      b->inactive = v->next;
    }
  }

  // Agressive local relabel
  void localRelabel(Node* v)
  {
#ifdef DEBUG/*{{{*/
    cout << "localRelabel(" << v - graph.nodes << ")" << endl;
#endif/*}}}*/
    unsigned int minDistance = graph.n;
    Edge* minEdge = NULL;
    v->distance = graph.n;

    workCounter += CONSTANT_WORK;

    // Find minimum distance edge
    for (Edge* e = v->firstEdge; e < (v+1)->firstEdge; e++) {
      if (e->capacity == 0 || e->target->distance >= minDistance) {
        continue;
      }

      workCounter++;
      minDistance = e->target->distance;
      minEdge = e;
    }

    minDistance++;

    // Set distance to 1 more than the minimum of all incident nodes
    if (minDistance < graph.n) {
      v->distance = minDistance;
      v->currentEdge = minEdge;

      if (maxLabel < minDistance) {
        maxLabel = minDistance;
      }
    }
  }

  // Gap heuristics
  void gapHeuristics(Bucket* b)
  {
#ifdef DEBUG/*{{{*/
    cout << "gapHeuristics(" << b - buckets << ")" << endl;
#endif/*}}}*/
    for (Bucket* c = b + 1; c <= &buckets[maxLabel]; c++) {
      for (Node* v = c->inactive; v != graph.sentinel; v = v->next) {
        v->distance = graph.n;
      }

      c->inactive = graph.sentinel;
    }

    // No need to look at those nodes anymore, set new max
    maxLabel = (b - buckets) - 1;
    maxActive = (b - buckets) - 1;
  }

  // Push flow out
  void pushOut(Node* v)
  {
#ifdef DEBUG/*{{{*/
    cout << "pushOut(" << v - graph.nodes << ")" << endl;
#endif/*}}}*/
    Edge* e;
    Bucket* bv;
    Bucket* bu;
    unsigned int delta;

    while (true) {
      bv = &buckets[v->distance]; // v->distance is changed in this loop

      for (e = v->currentEdge; e != (v+1)->firstEdge; e++) {
        if (e->capacity == 0) {
          continue;
        }

        Node* u = e->target;

        if (u->distance != v->distance - 1) {
          continue;
        }

        if (e->capacity < v->excess) {
          delta = e->capacity;
        } else {
          delta = v->excess;
        }

        e->reverse->capacity += delta;
        e->capacity -= delta;

        if (u != sink) {
          bu = &buckets[v->distance - 1];

          if (u->excess == 0) {
            inactiveRemove(bu, u);
            activeAdd(bu, u);
          }
        }

        u->excess += delta;
        v->excess -= delta;

        if (v->excess == 0) {
          break;
        }
      }

      if (e == (v+1)->firstEdge) {
        localRelabel(v);

        if (graph.n == v->distance) {
          break;
        }

        if ((bv->active == graph.sentinel) && (bv->inactive == graph.sentinel)) {
          // Gap heuristics: No node v with v->distance = i =>
          // Set all nodes u with u->distance > i: u->distance = graph.n
          gapHeuristics(bv);
        }

        if (graph.n == v->distance) {
          break;
        }
      } else {
        v->currentEdge = e;
        inactiveAdd(bv, v);
        break;
      }
    }
  }

  // Global relabel: reverse BFS from sink to reset all distances
  void globalRelabel()
  {
#ifdef DEBUG/*{{{*/
    cout << "globalRelabel()" << endl;
#endif/*}}}*/
    // Clear distances
    for (Node* v = graph.nodes; v != graph.sentinel; v++) {
      v->distance = graph.n;
    }

    sink->distance = 0;

    // Clear buckets
    for (Bucket* b = buckets; b <= &buckets[maxLabel]; b++) {
      b->active = graph.sentinel;
      b->inactive = graph.sentinel;
    }

    // Clear labels
    maxLabel = 0;
    maxActive = 0;
    minActive = graph.n;

    // Reverse BFS from sink
    inactiveAdd(buckets, sink);

    unsigned int currentDistance = 0;

    while (true) {
      Bucket* b = &buckets[currentDistance++];

      if ((b->active == graph.sentinel) && (b->inactive == graph.sentinel)) {
        break;
      }

      for (Node* v = b->active; v != graph.sentinel; v = v->next) {
        visit(v, currentDistance);
      }

      for (Node* v = b->inactive; v != graph.sentinel; v = v->next) {
        visit(v, currentDistance);
      }
    }
  }

  // reverse BFS: visit a node
  void visit(Node* v, unsigned int newDistance) {
    for (Edge* e = v->firstEdge; e < (v+1)->firstEdge; e++) {
      if (!e->reverse->capacity) {
        continue; // We only need edges that support a flow towards sink
      }

      Node* u = e->target;

      if (e->target->distance != graph.n) {
        continue; // Was already visited, new distance would be higher
      }

      u->distance = newDistance;
      u->currentEdge = u->firstEdge;

      if (newDistance > maxLabel) {
        maxLabel = newDistance;
      }

      if (u->excess > 0) {
        activeAdd(&buckets[newDistance], u);
      } else {
        inactiveAdd(&buckets[newDistance], u);
      }
    }
  }

public:
  FlowImplementation( const Parser & parser )
    : graph( parser )
    , updateLimit(graph.n * N_MULTIPLE + graph.m)
    , maxActive(0)
    , minActive(graph.n)
    , maxLabel(1)
    , workCounter(0)
  {
    buckets = (Bucket*) malloc((parser.num_nodes + 2) * sizeof(Bucket));

    for (Bucket* i = buckets; i < &buckets[graph.n-1]; i++) {
      i->active = graph.sentinel;
      i->inactive = graph.sentinel;
    }
  }

  //berechnet den Wert des Flusses zwischen Quelle (source) und Senke (sink)
  unsigned int calculateFlow( unsigned int sourceNr, unsigned int sinkNr ) {
    sink = &graph.nodes[sinkNr];
    source = &graph.nodes[sourceNr];

    source->distance = graph.n;

    // Saturate
    for (Edge* e = source->firstEdge; e < (source + 1)->firstEdge; e++) {
      e->reverse->capacity += e->capacity;
      e->target->excess += e->capacity;
      e->capacity = 0;
    }

    // Fill buckets, set labels
    for (Node* v = &graph.nodes[1]; v != graph.sentinel; v++) {
      if (v == sink) {
        v->distance = 0;
        inactiveAdd(buckets, v);
        continue;
      }

      if (v == source) {
        v->distance = graph.n;
      } else {
        v->distance = 1;
      }

      if (v->excess > 0) {
        activeAdd(buckets + 1, v);
      } else {
        inactiveAdd(buckets + 1, v);
      }
    }

    maxLabel = 1;

    globalRelabel();
    workCounter = 0;

    do {
#ifdef DEBUG/*{{{*/
      cout << "maxActive: " << maxActive << endl;
#endif/*}}}*/
      if (buckets[maxActive].active == graph.sentinel) {
        if (maxActive == 0) {
          break; // Sink not reachable from source
        }

        maxActive--;
      } else {
        Node* v = buckets[maxActive].active;
        activeRemove(&buckets[maxActive]);
        pushOut(v);

        if (maxActive < minActive) {
          break;
        }

        if (workCounter * UPDATE_FREQUENCY > updateLimit) {
          globalRelabel();
          workCounter = 0;
        }
      }
    } while (maxActive >= minActive);

    return sink->excess;
  }
};

#endif /* FLOW_IMPLEMENTATION_HPP_ */
