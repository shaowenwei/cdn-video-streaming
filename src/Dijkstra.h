#ifndef __DIJKSTRA_H__
#define __DIJKSTRA_H__

#include <vector>
#include <queue>
#include <list>
#include <iostream>

using namespace std;

#define INF 1000000

typedef pair<int, int> iPair;

class Graph
{
    int V; // vertices
    list< pair<int, int> > *adj; // weight pair for every edge
    
public:
    Graph(int V);
    void addEdge(int u, int v, int w); // function to add an edge to graph
    int shortestPath(int s, vector<int> servers); // prints shortest path from s
};

Graph::Graph(int V) // Allocates memory for adjacency list
{
    this->V = V;
    adj = new list<iPair> [V];
}

void Graph::addEdge(int u, int v, int w) // edge and distance
{
    adj[u].push_back(make_pair(v, w));
    adj[v].push_back(make_pair(u, w));
}

int Graph::shortestPath(int src, vector<int> servers) // Prints shortest paths from src to all other vertices
{
    priority_queue< iPair, vector<iPair>, greater<iPair> > pq; //create min heap using priority queue
    vector<int> dist(V, INF); // Create a vector for distances and initialize all distances as infinite
    pq.push(make_pair(0, src)); // Insert source itself and initialize its distance as 0.
    dist[src] = 0;
    priority_queue< iPair, vector<iPair>, greater<iPair> > path;
    
    while (!pq.empty())
    {
        int u = pq.top().second; //vertice
        pq.pop();
        list< pair<int, int> >::iterator i;
        for (i = adj[u].begin(); i != adj[u].end(); ++i)
        {
            int v = (*i).first; // adjacent label
            int weight = (*i).second; // distance
            if (dist[v] > dist[u] + weight)
            {
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    
    for (int i = 0; i != (int)servers.size(); ++i)
    {
        int des = servers[i];
        path.push(make_pair(dist[des], des));
    }
    return path.top().second;
};


#endif
