class Edge {
  var v2 : int;
  var faceNormal : float<3>;
  var next : *Edge;

  static Create(v2 : uint, faceNormal : float<3>, head : &*Edge) : *Edge {
    var edge = new Edge;
    edge.v2 = v2;
    edge.faceNormal = faceNormal;
    if (head != null) {
      edge.next = head;
    }
    head = edge;
    return edge;
  }
}

class Mesh<VertexType, IndexType> {
  Mesh(positions : &[]float<3>, triangles : &[][3]uint, creaseAngle : float) {
    vertices = [triangles.length * 3] new VertexType;
    indices = [triangles.length * 3] new IndexType;
    var edgesByFirstIndex = [positions.length] new *Edge;
    var normals = [triangles.length] new [3]float<3>;
    for (var i = 0; i < triangles.length; ++i) {
      var t = triangles[i];
      var p : [3]float<3>;
      for (var j = 0; j < 3; ++j) {
        p[j] = positions[t[j]];
      }
      var faceNormal = Math.normalize(Math.cross(p[1] - p[0], p[2] - p[0]));
      for (var j = 0; j < 3; ++j) {
        normals[i][j] = faceNormal;
        Edge.Create(t[(j + 1) % 3], faceNormal, &edgesByFirstIndex[t[j]]);
      }
    }
    var cosAngle = Math.cos(creaseAngle);
    for (var i = 0; i < triangles.length; ++i) {
      for (var j = 0; j < 3; ++j) {
        var v1 = triangles[i][j];
        for (var edge1 = edgesByFirstIndex[v1]; edge1 != null; edge1 = edge1.next) {
          for (var edge2 = edgesByFirstIndex[edge1.v2]; edge2 != null; edge2 = edge2.next) {
            if (edge2.v2 == v1) {
              if (Math.dot(edge1.faceNormal, edge2.faceNormal) > cosAngle) {
                normals[i][j] += edge2.faceNormal;
              }
            }
          }
        }
      }
    }
    var dstIndex = 0;
    for (var i = 0; i < triangles.length; ++i ) {
      for (var j = 0; j < 3; ++j) {
        var v : VertexType;
        v.position = positions[triangles[i][j]];
        v.normal = Math.normalize(normals[i][j]);
        vertices[dstIndex] = v;
        indices[dstIndex] = (IndexType) dstIndex;
        dstIndex++;
      }
    }
  }
  var vertices : *[]VertexType;
  var indices : *[]IndexType;
}
