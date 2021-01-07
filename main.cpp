#include <bits/stdc++.h>

using namespace std;


struct Node;


struct Edge{
    const int id;
    int cost;

    Node * n1;
    Node * n2;

    Edge(int id, long cost, Node * n1, Node * n2):id(id), cost(cost), n1(n1), n2(n2){};
    Edge(int id, Node * n1, Node * n2):id(id), n1(n1), n2(n2){};//for centroid purposes where path dont need the weight
};
struct Node{
    const int id;

    Edge * parentPath;
    vector<Edge *> connections;

    Edge * centroidParentPath;
    vector<Edge *> centroidConnections;

    int objectType;//if the node contains some kind of an object - this is it

    int subTreeSize;

    int level;
    int firstEuler;//euler is defined on the centroid tree!
    int lastEuler;

    map<Node *, long> parentsPathCosts;//after centroid decomposition

    map<Node *, map<int, int>> occurrencesOnPathToParent;//says that there are objects of <type, quantity> on path to parent

    map<int, pair<Node *, long>> subtreeDistances;//closest distance to the n type of restaurant with cost of

    Node(int id, int objectType): id(id), objectType(objectType){};
    Node(int id): id(id){};
};
struct SparseTable{
    vector<vector<Node *>> tab;

    SparseTable(const vector<Node *> &data){
        int height = ceil(log2(data.size()));
        //first row
        tab.emplace_back();
        for(int i = 0; i < data.size(); i++){
            tab[0].push_back(data[i]);
        }
        for(int i = 1; i < height; i ++){
            tab.emplace_back();
            for(int j = 0 ; j < data.size() - pow(2, i) + 1; j ++){
                int minRange = j, maxRange = j + pow(2, i) - 1;
                Node * min1 = minRangeQuery(j, j + pow(2, i - 1) - 1);
                Node * min2 = minRangeQuery(j + pow(2, i - 1), maxRange);
                tab.back().push_back((min1->level < min2->level ? min1 : min2));
            }
        }
    }

    Node * minRangeQuery(int min, int max) const{
        if(min > max){
            int tmp = max;
            max = min;
            min = tmp;
        }
        int y = log2(max - min + 1);
        int x1 = min, x2 = max - pow(2, y) + 1;
        return (tab[y][x1]->level < tab[y][x2]->level ? tab[y][x1] : tab[y][x2]);
    }
};
struct CentroidDecomposition{
    //decompose the graph, recurrent function
    static Node * centroidDecomposition(Node * root, Node * const startingPoint, set<Node *> &ancestors, int &edgeID){
        Node * centroidRoot = findCentroid(root, ancestors);//sub centroid root
        if(startingPoint != nullptr) {
            Edge *centroidEdge = new Edge(++edgeID, centroidRoot, startingPoint);

            centroidRoot->centroidParentPath = centroidEdge;
            centroidRoot->centroidConnections.push_back(centroidEdge);
            startingPoint->centroidConnections.push_back(centroidEdge);
        }
        ancestors.insert(centroidRoot);
        for(auto e : centroidRoot->connections){
            Node * another = (e->n1 != centroidRoot ? e->n1 : e->n2);
            if(ancestors.find(another) == ancestors.end()) {
                centroidDecomposition(another, centroidRoot, ancestors, edgeID);
            }
        }
        return centroidRoot;
    }
    //finding the centroid of the tree with omitting the starting point - finding the children of starting point
    static Node * findCentroid(Node * root, set<Node *> ancestors){
        Node * centroid = root;//assuming that root is a centroid. If it has no connections satisfying - it is

        for(auto e : root->connections){
            Node * another = (e->n1 != root ? e->n1 : e->n2);
            if((root->subTreeSize / 2) < another->subTreeSize && ancestors.find(another) == ancestors.end()){
                //swap subtreeSizes
                int newRootSubTreeSize = root->subTreeSize - another->subTreeSize;
                another->subTreeSize = root->subTreeSize;
                root->subTreeSize = newRootSubTreeSize;

                centroid = findCentroid(another, ancestors);

                //moving to another as it is now suspected to be a centroid
            }
        }
        return centroid;
    }

    //fills the order of the euler traversal of the tree. ON THE CENTROID!
    static void eulerTour(Node * root, vector<Node *> &eulerTourOrder, int level = 0){
        root->firstEuler = eulerTourOrder.size();
        root->lastEuler = eulerTourOrder.size();
        root->level = level;
        eulerTourOrder.push_back(root);
        for(auto e : root->centroidConnections){
            if(e != root->centroidParentPath){
                Node * another = (e->n1 != root ? e->n1 : e->n2);
                eulerTour(another, eulerTourOrder, level + 1);
                root->lastEuler = eulerTourOrder.size();
                eulerTourOrder.push_back(root);
            }
        }
    };

    //rooting the normal tree and calculating its subtree sizes
    static int rootAndSubTreeSizes(Node * root, Edge * comingFrom){
        root->parentPath = comingFrom;

        int sum = 1;
        for(auto e : root->connections){
            if(e != comingFrom){
                Node * another = (e->n1 != root ? e->n1 : e->n2);
                sum += rootAndSubTreeSizes(another, e);
            }
        }
        root->subTreeSize = sum;
        return sum;
    }

    static bool isAncestor(const Node * const n1, const Node * const n2, const SparseTable &sp){
        return (sp.minRangeQuery(n1->firstEuler, n2->lastEuler) == n1);
        //checks if n1 is an ancestor of n2
    }

    //gets all necessary properties - way from node to its root, occurrences of some type on its way
    static long getPath(Node * const n1, Node * const n2, const SparseTable &sp){
        Node * lca = sp.minRangeQuery(n1->firstEuler, n2->lastEuler);
        return n1->parentsPathCosts[lca] + n2->parentsPathCosts[lca];
    }
    static bool isPresentOnPath(Node * const n1, Node * const n2, const SparseTable &sp, int type){
        Node * lca = sp.minRangeQuery(n1->firstEuler, n2->lastEuler);
        return (n1->occurrencesOnPathToParent[lca].find(type) != n1->occurrencesOnPathToParent[lca].end()) ||
                (n2->occurrencesOnPathToParent[lca].find(type) != n2->occurrencesOnPathToParent[lca].end());
    }
    static void findProperties(Node * root, const SparseTable &sp, set<int> &visited){
        visited.insert(root->id);
        map<int,int> occurrences;
        propagateProperties(root, root, sp, visited, nullptr, 0, occurrences);
        for(auto e : root->centroidConnections){
            if(e != root->centroidParentPath){
                Node * another = (e->n1 != root ? e->n1 : e->n2);
                findProperties(another, sp, visited);
            }
        }
    }
private:
    static void propagateProperties(Node * n, Node * const root, const SparseTable &sp, set<int> &visited, Edge * comingFrom, long currentPathCost, map<int, int> &occurrencesOnPath){

        if(occurrencesOnPath.find(n->objectType) == occurrencesOnPath.end()){
            occurrencesOnPath[n->objectType] = 1;
        }else{
            occurrencesOnPath[n->objectType] ++;
        }
        n->occurrencesOnPathToParent[root] = occurrencesOnPath;
        n->parentsPathCosts[root] = currentPathCost;

        for(auto e : n->connections){
            Node * another = (e->n1 != n ? e->n1 : e->n2);
            if(e != comingFrom && visited.find(another->id) == visited.end()){
                propagateProperties(another, root, sp, visited, e, currentPathCost + e->cost, occurrencesOnPath);
            }
        }
        occurrencesOnPath[n->objectType] --;
        if(occurrencesOnPath[n->objectType] < 0){
            cout<<"ERRROR";
        }
        if(occurrencesOnPath[n->objectType] == 0){
            occurrencesOnPath.erase(n->objectType);
        }
    }
};



int main() {
    int nodesCount, queriesCount;
    cin >> nodesCount >> queriesCount;

    vector<Node *> nodes;
    for(int i = 0; i < nodesCount; i ++){
        int objectType;
        cin >> objectType;
        nodes.push_back(new Node(i, objectType));
    }
    for(int i = 0; i < nodesCount - 1; i ++){
        int n1ID, n2ID, cost = 1;
        cin >> n1ID >> n2ID;
        // n1ID--; n2ID--; uncomment for OI
        Node * n1 = nodes[n1ID];
        Node * n2 = nodes[n2ID];

        //default cost is set to 0. If the tree is weighted - just change it to the input
        Edge * e = new Edge(i, cost, n1, n2);
        n1->connections.push_back(e);
        n2->connections.push_back(e);
    }
    CentroidDecomposition::rootAndSubTreeSizes(nodes[0], nullptr);

    int edgeID = 0;
    set<Node *> ancestors;
    Node * centroidRoot = CentroidDecomposition::centroidDecomposition(nodes[0], nullptr, ancestors, edgeID);

    vector<Node *> eulerTour;
    CentroidDecomposition::eulerTour(centroidRoot, eulerTour);

    SparseTable sp = SparseTable(eulerTour);

    set<int> visited;
    CentroidDecomposition::findProperties(centroidRoot, sp, visited);

    cout<<CentroidDecomposition::isPresentOnPath(nodes[9], nodes[4], sp, 2);
    cout<<"";
    return 0;
}
