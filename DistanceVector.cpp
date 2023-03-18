#include <iostream>
#include <string>
#include <sstream>
#include <climits>
#include <map>

using namespace std;

// structure to store the properties of a node in the topology
struct Node
{
    string name;                            // stores the name of the node
    bool converged;                         // true when table is converged
    map<string,int> links;                  // stores the links and distance
    map<string,map<string,int>> table;      // stores the distance table
    map<string,map<string,int>> table_prev; // stores the distance table of previous cycle

    // constructor
    Node(string _name)
    {
        name = _name;
        converged = false;
    }
};

// global variables
map<string,Node*> nodes;    // holds all nodes in the topology
int t = 0;                  // records the current time cycle

// create a link from source to dest and append to the source node
// if link exist, update/remove the link and update distance table
// if link does not exist, make a pair and append to the source node and update distance table
void createLink(Node* source, Node* dest, int distance)
{
    // iterate through the links of the source node
    for(auto &link : source->links)
    {   
        // find existing link to destination
        if(link.first == dest->name)
        {
            if(distance == -1)
            {   
                // delete the link when input distance is -1
                source->links.erase(dest->name);

                // update distance table
                for(auto &row : source->table) row.second[dest->name] = distance;
            }
            else
            {
                // otherwise update the distance
                source->links[dest->name] = distance;

                // update distance table
                source->table[dest->name][dest->name] = distance;
            }
            return;
        }
    }

    // link does not exist in current topology
    if(distance != -1){
        // make new pair and append to source node
        source->links.insert(make_pair(dest->name,distance));

        // update distance table
        for(auto &row : source->table) row.second[dest->name] = INT_MAX;
        source->table[dest->name][dest->name] = distance;
    }
}

// extract the source, destination and distance from an input string
// string format: "source dest distance"
void extractLink(string input)
{
    stringstream ss(input);
    string source;      // name of the source node
    Node* source_ptr;   // pointer to the source node
    string dest;        // name of the dest node
    Node* dest_ptr;     // pointer to the dest node
    int distance;       // distance between source and dest

    // extract and format from string stream
    ss >> source >> dest >> distance;

    // iterate through all nodes
    for(auto &node : nodes)
    {
        if(node.first == source)
        {
            // source node found
            source_ptr = node.second;
        }
        else if(node.first == dest)
        {
            // destination node found
            dest_ptr = node.second;
        }
    }

    // create link on both nodes
    createLink(source_ptr,dest_ptr,distance);
    createLink(dest_ptr,source_ptr,distance);
}

// find the minimum cost and via route in a row of table
// returns empty string and infinity by default
pair<string,int> getMin(map<string,int> row)
{
    int min = INT_MAX;
    string via = "";

    // iterate through entries in the row
    for(auto &entry : row)
    {
        // ignore no connection (-1)
        if(entry.second < min && entry.second != -1)
        {
            // found route with less cost
            via = entry.first;
            min = entry.second;
        }
    }

    return make_pair(via,min);
}

// initialize the distance table of a node
void initTable(Node* node)
{
    // temporary variable for a row
    map<string,int> row;

    // clear table
    node->table.clear();

    // create empty rows of the table
    for(auto &n : nodes)
    {
        if(n.first != node->name) node->table.insert(make_pair(n.first,row));
    }
    
    // create columns of the table
    for(auto it = node->table.begin(); it != node->table.end(); it++)
    {
        // initialise to -1 to represent no connection
        for(auto &n : nodes)
        {
            if(n.first != node->name) it->second.insert(make_pair(n.first,-1));
        }

        for(auto &link : node->links)
        {
            if(it->first == link.first)
            {
                // set neighbors to link distance
                it->second[link.first] = link.second;
            }
            else
            {
                // set non neighbors to infinity
                it->second[link.first] = INT_MAX;
            }
        }
    }
}

// update the distance table of node
void updateTable(Node* node)
{
    // initialise to converged
    node->converged = true;

    // iterate through distance table
    for(auto &row : node->table)
    {
        for(auto &entry : row.second)
        {
            // skip if there is no connection
            if(row.first != entry.first && entry.second != -1)
            {
                // find the minimum via route min_v{D_v(y)
                pair<string,int> min = getMin(nodes[entry.first]->table_prev[row.first]);

                if(min.first == "")
                {
                    // no viable route is found
                    // set to infinity
                    entry.second = INT_MAX;
                    node->converged = false;
                }
                else
                {
                    // viable route found
                    // calculate D_x(y) = c(x,v) + min_v{D_v(y)}
                    int d = node->table[entry.first][entry.first] + min.second;

                    if(d != entry.second)
                    {
                        // route cost changed
                        entry.second = d;
                        node->converged = false;
                    }
                }
            }
        }
    }
}

// print the distance table of node
void printTable(Node* node)
{
    // print router and time
    cout << "router " << node->name << " at t=" << t << endl;

    // print column headers
    for(auto &row : node->table) cout << "\t" << row.first;
    cout << endl;

    // iterate through the table
    for(auto &row : node->table)
    {
        cout << row.first;
        for(auto &entry : row.second)
        {
            if(entry.second == INT_MAX)
            {
                cout << "\t" << "INF";
            }
            else if(entry.second == -1)
            {
                cout << "\t" << "-";
            }
            else
            {
                cout << "\t" << entry.second;
            }
        }
        cout << endl;
    }
    cout << endl;
}

// print the route for all routers
void printRoutes()
{
    // temp variable
    pair<string,int> tmp;

    // iterate through all routers
    for(auto &node : nodes)
    {
        // iterate through the distance table
        for(auto &row : node.second->table)
        {
            cout << "router " << node.first << ": ";

            tmp = getMin(row.second);

            if(tmp.second == INT_MAX)
            {
                cout << row.first << " is unreachable" << endl;
            }
            else
            {
                cout << row.first << " is " << tmp.second << " routing through " << tmp.first << endl;
            }
        }
    }
    cout << endl;
}

// returns true if all nodes are converged
bool isConverged()
{
    // return false if a node is not converged
    for(auto &node : nodes) if(!node.second->converged) return false;

    return true;
} 

// distance vector algorithm main function
// update == false for new topology
// update == true for updated topology
void distanceVector(bool update)
{
    if(!update)
    {
        // initialise distance table for new topology
        for(auto &node : nodes) initTable(node.second);
    }
    else
    {
        // run the algorithm for updated topology
        for(auto &node : nodes) updateTable(node.second);
    }

    // repeat until the topology is converged
    while(!isConverged())
    {
        // backup current distance table
        for(auto &node : nodes) node.second->table_prev = node.second->table;

        // iterate through all routers
        for(auto &node : nodes)
        {
            printTable(node.second);
            updateTable(node.second);
        }

        // increment time
        t++;
    }

    // print routes of each router
    printRoutes();
}

// main function
int main()
{
    string tmp;                 // temporary string for reading
    bool update = false;        // becomes true if the input contains updated topology

    // read the name of each router/node in the topology
    while(getline(cin,tmp))
    {
        // stop at blank line
        if(tmp.empty()) break;

        // push node name into vector and increment counter
        nodes.insert(make_pair(tmp,new Node(tmp)));
    }

    // read the details of each link/edge in the topology
    while(getline(cin,tmp))
    {
        // stop at blank line
        if(tmp.empty()) break;

        // extract link from input and push into source node
        extractLink(tmp);
    }

    // execute algorithm with topology here
    distanceVector(false);

    // read the updates to each link/edge in the topology
    while(getline(cin,tmp))
    {
        // stop at blank line
        if(tmp.empty()) break;

        // update current topology from input
        update = true;
        extractLink(tmp);
    }

    // execute algorithm again with updated topology here (if there is any)
    if(update) distanceVector(true);

    // delete nodes from heap
    for(auto &node : nodes) delete node.second;

    return 0;
}