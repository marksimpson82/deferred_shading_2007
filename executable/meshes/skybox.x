xof 0303txt 0032
template KeyValuePair {
 <26e6b1c3-3d4d-4a1d-a437-b33668ffa1c2>
 STRING key;
 STRING value;
}

template Frame {
 <3d82ab46-62da-11cf-ab39-0020af71e433>
 [...]
}

template Matrix4x4 {
 <f6f23f45-7686-11cf-8f52-0040333594a3>
 array FLOAT matrix[16];
}

template FrameTransformMatrix {
 <f6f23f41-7686-11cf-8f52-0040333594a3>
 Matrix4x4 frameMatrix;
}

template ObjectMatrixComment {
 <95a48e28-7ef4-4419-a16a-ba9dbdf0d2bc>
 Matrix4x4 objectMatrix;
}

template Vector {
 <3d82ab5e-62da-11cf-ab39-0020af71e433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template MeshFace {
 <3d82ab5f-62da-11cf-ab39-0020af71e433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template Mesh {
 <3d82ab44-62da-11cf-ab39-0020af71e433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

template MeshNormals {
 <f6f23f43-7686-11cf-8f52-0040333594a3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template Coords2d {
 <f6f23f44-7686-11cf-8f52-0040333594a3>
 FLOAT u;
 FLOAT v;
}

template MeshTextureCoords {
 <f6f23f40-7686-11cf-8f52-0040333594a3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template ColorRGBA {
 <35ff44e0-6c7c-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template ColorRGB {
 <d3e16e81-7835-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template Material {
 <3d82ab4d-62da-11cf-ab39-0020af71e433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshMaterialList {
 <f6f23f42-7686-11cf-8f52-0040333594a3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material <3d82ab4d-62da-11cf-ab39-0020af71e433>]
}

template TextureFilename {
 <a42790e1-7810-11cf-8f52-0040333594a3>
 STRING filename;
}

template AnimTicksPerSecond {
 <9e415a43-7ba6-4a73-8743-b73d47e88476>
 DWORD AnimTicksPerSecond;
}

template Animation {
 <3d82ab4f-62da-11cf-ab39-0020af71e433>
 [...]
}

template AnimationSet {
 <3d82ab50-62da-11cf-ab39-0020af71e433>
 [Animation <3d82ab4f-62da-11cf-ab39-0020af71e433>]
}

template AnimationOptions {
 <e2bf56c0-840f-11cf-8f52-0040333594a3>
 DWORD openclosed;
 DWORD positionquality;
}

template FloatKeys {
 <10dd46a9-775b-11cf-8f52-0040333594a3>
 DWORD nValues;
 array FLOAT values[nValues];
}

template TimedFloatKeys {
 <f406b180-7b3b-11cf-8f52-0040333594a3>
 DWORD time;
 FloatKeys tfkeys;
}

template AnimationKey {
 <10dd46a8-775b-11cf-8f52-0040333594a3>
 DWORD keyType;
 DWORD nKeys;
 array TimedFloatKeys keys[nKeys];
}


KeyValuePair {
 "Date";
 "2007-05-01 17:59:59";
}

KeyValuePair {
 "File";
 "D:\\3dsMax8\\scenes\\uni\\deferred shading\\skybox\\skybox.max";
}

KeyValuePair {
 "User";
 "Mark";
}

KeyValuePair {
 "CoreTime";
 "0";
}

Frame Box01 {
 

 FrameTransformMatrix relative {
  1.000000,0.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,-1.000000,0.000000,0.000000,0.000000,-0.000000,100.000000,1.000000;;
 }

 ObjectMatrixComment object {
  1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000;;
 }

 Mesh mesh_Box01 {
  24;
  -100.000000;0.000008;100.000000;,
  100.000000;0.000008;100.000000;,
  100.000000;-0.000008;-100.000000;,
  -100.000000;-0.000008;-100.000000;,
  -100.000000;-200.000000;100.000008;,
  -100.000000;-200.000000;-99.999992;,
  100.000000;-200.000000;-99.999992;,
  100.000000;-200.000000;100.000008;,
  -100.000000;0.000008;100.000000;,
  -100.000000;-200.000000;100.000008;,
  100.000000;-200.000000;100.000008;,
  100.000000;0.000008;100.000000;,
  100.000000;0.000008;100.000000;,
  100.000000;-200.000000;100.000008;,
  100.000000;-200.000000;-99.999992;,
  100.000000;-0.000008;-100.000000;,
  100.000000;-0.000008;-100.000000;,
  100.000000;-200.000000;-99.999992;,
  -100.000000;-200.000000;-99.999992;,
  -100.000000;-0.000008;-100.000000;,
  -100.000000;-0.000008;-100.000000;,
  -100.000000;-200.000000;-99.999992;,
  -100.000000;-200.000000;100.000008;,
  -100.000000;0.000008;100.000000;;
  12;
  3;1,0,2;,
  3;3,2,0;,
  3;5,4,6;,
  3;7,6,4;,
  3;9,8,10;,
  3;11,10,8;,
  3;13,12,14;,
  3;15,14,12;,
  3;17,16,18;,
  3;19,18,16;,
  3;21,20,22;,
  3;23,22,20;;

  MeshNormals normals {
   24;
   0.000000;-1.000000;0.000000;,
   0.000000;-1.000000;0.000000;,
   0.000000;-1.000000;0.000000;,
   0.000000;-1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;-0.000000;-1.000000;,
   0.000000;-0.000000;-1.000000;,
   0.000000;-0.000000;-1.000000;,
   0.000000;-0.000000;-1.000000;,
   -1.000000;0.000000;0.000000;,
   -1.000000;0.000000;0.000000;,
   -1.000000;0.000000;0.000000;,
   -1.000000;0.000000;0.000000;,
   0.000000;0.000000;1.000000;,
   0.000000;0.000000;1.000000;,
   0.000000;0.000000;1.000000;,
   0.000000;0.000000;1.000000;,
   1.000000;0.000000;0.000000;,
   1.000000;0.000000;0.000000;,
   1.000000;0.000000;0.000000;,
   1.000000;0.000000;0.000000;;
   12;
   3;1,0,2;,
   3;3,2,0;,
   3;5,4,6;,
   3;7,6,4;,
   3;9,8,10;,
   3;11,10,8;,
   3;13,12,14;,
   3;15,14,12;,
   3;17,16,18;,
   3;19,18,16;,
   3;21,20,22;,
   3;23,22,20;;
  }

  MeshTextureCoords tc0 {
   24;
   0.000000;1.000000;,
   1.000000;1.000000;,
   1.000000;-0.000000;,
   0.000000;0.000000;,
   1.000000;1.000000;,
   1.000000;0.000000;,
   0.000000;0.000000;,
   0.000000;1.000000;,
   1.000000;-0.000000;,
   0.000000;-0.000000;,
   -0.000000;1.000000;,
   1.000000;1.000000;,
   0.000000;1.000000;,
   1.000000;1.000000;,
   1.000000;0.000000;,
   -0.000000;0.000000;,
   1.000000;0.000000;,
   0.000000;-0.000000;,
   0.000000;1.000000;,
   1.000000;1.000000;,
   1.000000;0.000000;,
   0.000000;0.000000;,
   0.000000;1.000000;,
   1.000000;1.000000;;
  }

  MeshMaterialList mtls {
   6;
   12;
   0,
   0,
   1,
   1,
   2,
   2,
   3,
   3,
   4,
   4,
   5,
   5;

   Material Standard_5 {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "west.dds";
    }
   }

   Material Standard_2s {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "east.dds";
    }
   }

   Material Standard_8 {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "down.dds";
    }
   }

   Material Standard_2 {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "north.dds";
    }
   }

   Material Standard_7 {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "up.dds";
    }
   }

   Material Standard_4 {
    0.588000;0.588000;0.588000;1.000000;;
    10.000000;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "south.dds";
    }
   }
  }
 }
}

AnimTicksPerSecond fps {
 4800;
}

AnimationSet Idle {
 

 Animation Anim-Idle-Box01 {
  
  { Box01 }

  AnimationOptions {
   1;
   0;
  }

  AnimationKey rot {
   0;
   1;
   0;4;0.707107,-0.707107,0.000000,0.000000;;;
  }

  AnimationKey scale {
   1;
   1;
   0;3;1.000000,1.000000,1.000000;;;
  }

  AnimationKey pos {
   2;
   1;
   0;3;0.000000,-0.000000,100.000000;;;
  }
 }
}