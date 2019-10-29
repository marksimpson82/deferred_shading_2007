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
 "2007-04-27 15:26:05";
}

KeyValuePair {
 "File";
 "D:\\3dsMax8\\scenes\\uni\\deferred shading\\generator_room\\interior2.max";
}

KeyValuePair {
 "User";
 "Mark";
}

KeyValuePair {
 "CoreTime";
 "0";
}

Frame raised_pillar {
 

 FrameTransformMatrix relative {
  4.207069,0.000000,0.000000,0.000000,0.000000,1.000000,-0.000000,0.000000,0.000000,0.000000,2.621322,0.000000,0.000000,0.000000,0.000005,1.000000;;
 }

 ObjectMatrixComment object {
  1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,-0.000000,0.000000,0.000000,-0.000000,1.000000,0.000000,0.000000,127.999985,128.000000,1.000000;;
 }

 Mesh mesh_raised_pillar {
  30;
  -128.000000;-0.000015;42.666672;,
  -128.000000;14.999986;42.666672;,
  128.000000;14.999986;42.666653;,
  128.000000;-0.000015;42.666656;,
  128.000000;-0.000015;-42.666668;,
  128.000000;14.999985;-42.666668;,
  -128.000000;14.999985;-42.666656;,
  -128.000000;-0.000015;-42.666656;,
  -128.000000;14.999986;42.666672;,
  -128.000000;15.999986;41.666672;,
  -79.360001;15.999986;41.666672;,
  128.000000;14.999986;42.666653;,
  -128.000000;14.999986;42.666672;,
  -79.360001;15.999986;41.666672;,
  128.000000;15.999986;41.666656;,
  128.000000;14.999986;42.666653;,
  -79.360001;15.999986;41.666672;,
  128.000000;14.999985;-42.666668;,
  128.000000;15.999986;-41.666668;,
  -79.360001;15.999986;-41.666656;,
  -128.000000;14.999985;-42.666656;,
  128.000000;14.999985;-42.666668;,
  -79.360001;15.999986;-41.666656;,
  -128.000000;15.999986;-41.666656;,
  -128.000000;14.999985;-42.666656;,
  -79.360001;15.999986;-41.666656;,
  128.000000;15.999986;-41.666668;,
  128.000000;15.999986;41.666656;,
  -79.360001;15.999986;41.666672;,
  -79.360001;15.999986;-41.666656;;
  12;
  3;1,0,2;,
  3;3,2,0;,
  3;5,4,6;,
  3;7,6,4;,
  3;9,8,10;,
  3;12,11,13;,
  3;15,14,16;,
  3;18,17,19;,
  3;21,20,22;,
  3;24,23,25;,
  3;27,26,28;,
  3;29,28,26;;

  MeshNormals normals {
   30;
   0.000000;0.000000;0.381487;,
   0.000000;0.000000;0.381487;,
   0.000000;0.000000;0.381487;,
   0.000000;0.000000;0.381487;,
   -0.000000;0.000000;-0.381487;,
   -0.000000;0.000000;-0.381487;,
   -0.000000;0.000000;-0.381487;,
   -0.000000;0.000000;-0.381487;,
   0.000000;0.707107;0.269752;,
   0.000000;0.707107;0.269752;,
   0.000000;0.707107;0.269752;,
   0.000000;0.707106;0.269752;,
   0.000000;0.707106;0.269752;,
   0.000000;0.707106;0.269752;,
   0.000000;0.707107;0.269752;,
   0.000000;0.707107;0.269752;,
   0.000000;0.707107;0.269752;,
   -0.000000;0.707107;-0.269752;,
   -0.000000;0.707107;-0.269752;,
   -0.000000;0.707107;-0.269752;,
   -0.000000;0.707108;-0.269752;,
   -0.000000;0.707108;-0.269752;,
   -0.000000;0.707108;-0.269752;,
   0.000000;0.707107;-0.269752;,
   0.000000;0.707107;-0.269752;,
   0.000000;0.707107;-0.269752;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;,
   0.000000;1.000000;0.000000;;
   12;
   3;1,0,2;,
   3;3,2,0;,
   3;5,4,6;,
   3;7,6,4;,
   3;9,8,10;,
   3;12,11,13;,
   3;15,14,16;,
   3;18,17,19;,
   3;21,20,22;,
   3;24,23,25;,
   3;27,26,28;,
   3;29,28,26;;
  }

  MeshTextureCoords tc0 {
   30;
   -0.000022;-0.134297;,
   -0.000022;-0.061120;,
   4.779496;-0.061120;,
   4.779496;-0.134297;,
   4.779496;-0.133843;,
   4.779496;-0.061321;,
   -0.000022;-0.061321;,
   -0.000022;-0.133843;,
   -0.000022;-0.061120;,
   -0.000022;-0.049402;,
   0.908147;-0.049402;,
   4.779496;-0.061120;,
   -0.000022;-0.061120;,
   0.908147;-0.049402;,
   4.779496;-0.049402;,
   4.779496;-0.061120;,
   0.908147;-0.049402;,
   4.779496;-0.061321;,
   4.779496;-0.049603;,
   0.908147;-0.049602;,
   -0.000022;-0.061321;,
   4.779496;-0.061321;,
   0.908147;-0.049602;,
   -0.000022;-0.049602;,
   -0.000022;-0.061321;,
   0.908147;-0.049602;,
   4.779496;-0.049603;,
   4.779496;0.926960;,
   0.908147;0.926960;,
   0.908147;-0.049602;;
  }

  MeshMaterialList mtls {
   1;
   12;
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0;

   Material _07___Default {
    0.588235;0.588235;0.588235;1.000000;;
    9.999999;
    0.900000;0.900000;0.900000;;
    0.000000;0.000000;0.000000;;

    TextureFilename Diffuse {
     "ff_wall_cement16-2.dds";
    }
   }
  }
 }
}

AnimTicksPerSecond fps {
 4800;
}

AnimationSet Idle {
 

 Animation Anim-Idle-raised_pillar {
  
  { raised_pillar }

  AnimationOptions {
   1;
   0;
  }

  AnimationKey rot {
   0;
   1;
   0;4;1.000000,0.000000,0.000000,0.000000;;;
  }

  AnimationKey scale {
   1;
   1;
   0;3;4.207069,1.000000,2.621322;;;
  }

  AnimationKey pos {
   2;
   1;
   0;3;0.000000,0.000000,0.000005;;;
  }
 }
}