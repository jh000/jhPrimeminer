typedef struct  
{
	float x;
	float y;
}point2D_t;

typedef struct  
{
	float x;
	float y;
	float z;
}point3D_t;

typedef struct  
{
	float x;
	float y;
	float z;
	float w;
}point4D_t;

typedef point3D_t vector3D_t;
typedef point2D_t vector2D_t;
typedef point4D_t vector4D_t;

typedef struct _aabb_t
{
	vector3D_t low;
	vector3D_t high;
}aabb_t;


void spline_pointOnCurve(point2D_t* out, float t, point2D_t* p0, point2D_t* p1, point2D_t* p2, point2D_t* p3);

// matrices
void matrix4x4_multiplyOpenGL(float *result, float *matrix1, float *matrix2);

void matrix_buildMatrix(vector4D_t matrixOut[4], vector3D_t* translation, vector3D_t* scale, vector3D_t* rotation);
void vector_multiplyMatrix(vector3D_t* vector, vector4D_t matrix[4]);

// vector 2D
void vector_sub(vector2D_t *dst, vector2D_t *a, vector2D_t *b);
void vector_copy(vector2D_t *dst, vector2D_t *src);
void vector_rotate(vector2D_t *dst, vector2D_t *a, float angle);
void vector_scale(vector2D_t *dst, float factor);
void vector_scale(vector2D_t *dst, vector2D_t *a, float factor);
void vector_rotate(vector2D_t *v, float angle);
void vector_addMul(vector2D_t *dst, vector2D_t *a, vector2D_t *b, float factor);
float vector_dot(vector2D_t *a, vector2D_t *b);
void vector_normalize(vector2D_t *v);
float vector_length(vector2D_t *v);
float vector_distance(vector2D_t *v1, vector2D_t *v2);

// vector 3D
void vector_sub(vector3D_t *dst, vector3D_t *a, vector3D_t *b);
void vector_copy(vector3D_t *dst, vector3D_t *src);
void vector_scale(vector3D_t *dst, vector3D_t *a, float factor);
float vector_distance(vector3D_t *v1, vector3D_t *v2);
float vector_distanceSquare(vector3D_t *v1, vector3D_t *v2);
void vector_cross(vector3D_t *dst, vector3D_t *a, vector3D_t *b);
float vector_dot(vector3D_t *a, vector3D_t *b);
void vector_normalize(vector3D_t *v);
float vector_length(vector3D_t *v);
void vector_rotateXZ(vector3D_t *dst, vector3D_t *a, float angle);

// special, vector 2D & 3D
void vector_calculateNormal(vector3D_t* normalOutput, vector3D_t* v1, vector3D_t* v2, vector3D_t* v3);
void vector_calculateNormal(vector3D_t* normalOutput, vector3D_t* dif1, vector3D_t* dif2);

float vector_distanceRayToPoint(vector3D_t* origin, vector3D_t* direction, vector3D_t* point);
float vector_projectPointOnRay(vector3D_t* origin, vector3D_t* direction, vector3D_t* point);
float vector_projectPointOnRay(vector2D_t* origin, vector2D_t* direction, vector2D_t* point);
// perlin noise
void perlinNoise_init();
float perlinNoise_1d(float x);
float perlinNoise_2d(float x, float y);
float perlinNoise_3d(float x, float y, float z);

float perlinNoise_2d_q2(float x, float y);

// random
float random_float01();

// intersection
bool intersection_rayBox(vector3D_t* orig, vector3D_t* dir, vector3D_t* box_max, vector3D_t* box_min, float* distance);
bool intersection_rayBox(vector3D_t* orig, vector3D_t* dir, vector3D_t* box_max, vector3D_t* box_min, float* distance, uint8* intersectionFace);

inline float _fastAbs(float v);

// bitstuff
uint32 getLeastBitIndex(uint32 mask);
