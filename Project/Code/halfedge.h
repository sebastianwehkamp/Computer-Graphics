#ifndef HALFEDGE
#define HALFEDGE

// Forward declarations
class Vertex;
class Face;

class HalfEdge {

public:
  Vertex* target;
  HalfEdge* next;
  HalfEdge* prev;
  HalfEdge* twin;
  Face* polygon;
  unsigned int index;
  float sharpness;
  bool selected;

  // Inline constructors

  HalfEdge() {
    target = nullptr;
    next = nullptr;
    prev = nullptr;
    twin = nullptr;
    polygon = nullptr;
    index = 0;
    sharpness = 0;
    selected = false;
  }

  HalfEdge(Vertex* htarget, HalfEdge* hnext, HalfEdge* hprev, HalfEdge* htwin, Face* hpolygon, unsigned int hindex, float hsharpness=0, bool hselected=false) {
    target = htarget;
    next = hnext;
    prev = hprev;
    twin = htwin;
    polygon = hpolygon;
    index = hindex;
    sharpness = hsharpness;
    selected = hselected;
  }
};

#endif // HALFEDGE
