#include"JHLib.h"

void spline_pointOnCurve(point2D_t* out, float t, point2D_t* p0, point2D_t* p1, point2D_t* p2, point2D_t* p3)
{
	float t2 = t * t;
	float t3 = t2 * t;
	out->x = 0.5f * ( ( 2.0f * p1->x ) +
		( -p0->x + p2->x ) * t +
		( 2.0f * p0->x - 5.0f * p1->x + 4 * p2->x - p3->x ) * t2 +
		( -p0->x + 3.0f * p1->x - 3.0f * p2->x + p3->x ) * t3 );
	out->y = 0.5f * ( ( 2.0f * p1->y ) +
		( -p0->y + p2->y ) * t +
		( 2.0f * p0->y - 5.0f * p1->y + 4 * p2->y - p3->y ) * t2 +
		( -p0->y + 3.0f * p1->y - 3.0f * p2->y + p3->y ) * t3 );
} 

/*
 * Multiply matrix the OpenGL way
 */
void matrix4x4_multiplyOpenGL(float *result, float *matrix1, float *matrix2)
{
	result[0]=matrix1[0]*matrix2[0]+
		matrix1[4]*matrix2[1]+
		matrix1[8]*matrix2[2]+
		matrix1[12]*matrix2[3];
	result[4]=matrix1[0]*matrix2[4]+
		matrix1[4]*matrix2[5]+
		matrix1[8]*matrix2[6]+
		matrix1[12]*matrix2[7];
	result[8]=matrix1[0]*matrix2[8]+
		matrix1[4]*matrix2[9]+
		matrix1[8]*matrix2[10]+
		matrix1[12]*matrix2[11];
	result[12]=matrix1[0]*matrix2[12]+
		matrix1[4]*matrix2[13]+
		matrix1[8]*matrix2[14]+
		matrix1[12]*matrix2[15];
	result[1]=matrix1[1]*matrix2[0]+
		matrix1[5]*matrix2[1]+
		matrix1[9]*matrix2[2]+
		matrix1[13]*matrix2[3];
	result[5]=matrix1[1]*matrix2[4]+
		matrix1[5]*matrix2[5]+
		matrix1[9]*matrix2[6]+
		matrix1[13]*matrix2[7];
	result[9]=matrix1[1]*matrix2[8]+
		matrix1[5]*matrix2[9]+
		matrix1[9]*matrix2[10]+
		matrix1[13]*matrix2[11];
	result[13]=matrix1[1]*matrix2[12]+
		matrix1[5]*matrix2[13]+
		matrix1[9]*matrix2[14]+
		matrix1[13]*matrix2[15];
	result[2]=matrix1[2]*matrix2[0]+
		matrix1[6]*matrix2[1]+
		matrix1[10]*matrix2[2]+
		matrix1[14]*matrix2[3];
	result[6]=matrix1[2]*matrix2[4]+
		matrix1[6]*matrix2[5]+
		matrix1[10]*matrix2[6]+
		matrix1[14]*matrix2[7];
	result[10]=matrix1[2]*matrix2[8]+
		matrix1[6]*matrix2[9]+
		matrix1[10]*matrix2[10]+
		matrix1[14]*matrix2[11];
	result[14]=matrix1[2]*matrix2[12]+
		matrix1[6]*matrix2[13]+
		matrix1[10]*matrix2[14]+
		matrix1[14]*matrix2[15];
	result[3]=matrix1[3]*matrix2[0]+
		matrix1[7]*matrix2[1]+
		matrix1[11]*matrix2[2]+
		matrix1[15]*matrix2[3];
	result[7]=matrix1[3]*matrix2[4]+
		matrix1[7]*matrix2[5]+
		matrix1[11]*matrix2[6]+
		matrix1[15]*matrix2[7];
	result[11]=matrix1[3]*matrix2[8]+
		matrix1[7]*matrix2[9]+
		matrix1[11]*matrix2[10]+
		matrix1[15]*matrix2[11];
	result[15]=matrix1[3]*matrix2[12]+
		matrix1[7]*matrix2[13]+
		matrix1[11]*matrix2[14]+
		matrix1[15]*matrix2[15];
}


void matrix4x4_multiply(float *result, float *matrix1, float *matrix2)
{
	
	result[0]=matrix1[0]*matrix2[0]+
		matrix1[1]*matrix2[4]+
		matrix1[2]*matrix2[8]+
		matrix1[3]*matrix2[12];
	result[1]=matrix1[0]*matrix2[1]+
		matrix1[1]*matrix2[5]+
		matrix1[2]*matrix2[9]+
		matrix1[3]*matrix2[13];
	result[2]=matrix1[0]*matrix2[2]+
		matrix1[1]*matrix2[6]+
		matrix1[2]*matrix2[10]+
		matrix1[3]*matrix2[14];
	result[3]=matrix1[0]*matrix2[3]+
		matrix1[1]*matrix2[7]+
		matrix1[2]*matrix2[11]+
		matrix1[3]*matrix2[15];
	result[4]=matrix1[4]*matrix2[0]+
		matrix1[5]*matrix2[4]+
		matrix1[6]*matrix2[8]+
		matrix1[7]*matrix2[12];
	result[5]=matrix1[4]*matrix2[1]+
		matrix1[5]*matrix2[5]+
		matrix1[6]*matrix2[9]+
		matrix1[7]*matrix2[13];
	result[6]=matrix1[4]*matrix2[2]+
		matrix1[5]*matrix2[6]+
		matrix1[6]*matrix2[10]+
		matrix1[7]*matrix2[14];
	result[7]=matrix1[4]*matrix2[3]+
		matrix1[5]*matrix2[7]+
		matrix1[6]*matrix2[11]+
		matrix1[7]*matrix2[15];
	result[8]=matrix1[8]*matrix2[0]+
		matrix1[9]*matrix2[4]+
		matrix1[10]*matrix2[8]+
		matrix1[11]*matrix2[12];
	result[9]=matrix1[8]*matrix2[1]+
		matrix1[9]*matrix2[5]+
		matrix1[10]*matrix2[9]+
		matrix1[11]*matrix2[13];
	result[10]=matrix1[8]*matrix2[2]+
		matrix1[9]*matrix2[6]+
		matrix1[10]*matrix2[10]+
		matrix1[11]*matrix2[14];
	result[11]=matrix1[8]*matrix2[3]+
		matrix1[9]*matrix2[7]+
		matrix1[10]*matrix2[11]+
		matrix1[11]*matrix2[15];
	result[12]=matrix1[12]*matrix2[0]+
		matrix1[13]*matrix2[4]+
		matrix1[14]*matrix2[8]+
		matrix1[15]*matrix2[12];
	result[13]=matrix1[12]*matrix2[1]+
		matrix1[13]*matrix2[5]+
		matrix1[14]*matrix2[9]+
		matrix1[15]*matrix2[13];
	result[14]=matrix1[12]*matrix2[2]+
		matrix1[13]*matrix2[6]+
		matrix1[14]*matrix2[10]+
		matrix1[15]*matrix2[14];
	result[15]=matrix1[12]*matrix2[3]+
		matrix1[13]*matrix2[7]+
		matrix1[14]*matrix2[11]+
		matrix1[15]*matrix2[15];
}

void matrix4x4_multiply(vector4D_t result[4], vector4D_t matrix1[4], vector4D_t matrix2[4])
{
	matrix4x4_multiply((float*)result, (float*)matrix1, (float*)matrix2);
}

//#define mmA(x,y) (x*4+y)

void matrix_buildMatrix(vector4D_t matrixOut[4], vector3D_t* translation, vector3D_t* scale, vector3D_t* rotation)
{
	vector4D_t calcMatrix[4];
	vector4D_t calcMatrix2[4];

	RtlZeroMemory(calcMatrix2, 4*4*sizeof(float));
	calcMatrix2[0].x = scale->x;
	calcMatrix2[1].y = scale->y;
	calcMatrix2[2].z = scale->z;
	calcMatrix2[3].w = 1.0f;

	// multiplication X
	vector4D_t rotMatrix[4];
	//float conv = (float)6.28318f / 360.0f;
	float c = cos(rotation->x);
	float s = sin(rotation->x);
	float rx = 1.0f;
	float ry = 0.0f;
	float rz = 0.0f;

	rotMatrix[0].x = rx*rx * (1.0f - c) + c;
	rotMatrix[1].x = rx*ry * (1.0f - c) - rz * s;
	rotMatrix[2].x = rx*rz * (1.0f - c) + ry * s;
	rotMatrix[3].x = 0.0f;

	rotMatrix[0].y = ry*rx * (1.0f - c) + rz * s;
	rotMatrix[1].y = ry*ry * (1.0f - c) + c;
	rotMatrix[2].y = ry*rz * (1.0f - c) - rx * s;
	rotMatrix[3].y = 0.0f;

	rotMatrix[0].z = rz*rx * (1.0f - c) - ry * s;
	rotMatrix[1].z = rz*ry * (1.0f - c) + rx * s;
	rotMatrix[2].z = rz*rz * (1.0f - c) + c;
	rotMatrix[3].z = 0.0f;

	rotMatrix[0].w = 0.0f;
	rotMatrix[1].w = 0.0f;
	rotMatrix[2].w = 0.0f;
	rotMatrix[3].w = 1.0f;
	matrix4x4_multiply(calcMatrix, calcMatrix2, rotMatrix);

	// multiplication Y
	c = cos(rotation->y);
	s = sin(rotation->y);
	rx = 0.0f;
	ry = 1.0f;
	rz = 0.0f;

	rotMatrix[0].x = rx*rx * (1.0f - c) + c;
	rotMatrix[1].x = rx*ry * (1.0f - c) - rz * s;
	rotMatrix[2].x = rx*rz * (1.0f - c) + ry * s;
	rotMatrix[3].x = 0.0f;

	rotMatrix[0].y = ry*rx * (1.0f - c) + rz * s;
	rotMatrix[1].y = ry*ry * (1.0f - c) + c;
	rotMatrix[2].y = ry*rz * (1.0f - c) - rx * s;
	rotMatrix[3].y = 0.0f;

	rotMatrix[0].z = rz*rx * (1.0f - c) - ry * s;
	rotMatrix[1].z = rz*ry * (1.0f - c) + rx * s;
	rotMatrix[2].z = rz*rz * (1.0f - c) + c;
	rotMatrix[3].z = 0.0f;

	rotMatrix[0].w = 0.0f;
	rotMatrix[1].w = 0.0f;
	rotMatrix[2].w = 0.0f;
	rotMatrix[3].w = 1.0f;
	matrix4x4_multiply(calcMatrix2, calcMatrix, rotMatrix);

	// multiplication Z
	c = cos(rotation->z);
	s = sin(rotation->z);
	rx = 0.0f;
	ry = 0.0f;
	rz = 1.0f;

	rotMatrix[0].x = rx*rx * (1.0f - c) + c;
	rotMatrix[1].x = rx*ry * (1.0f - c) - rz * s;
	rotMatrix[2].x = rx*rz * (1.0f - c) + ry * s;
	rotMatrix[3].x = 0.0f;

	rotMatrix[0].y = ry*rx * (1.0f - c) + rz * s;
	rotMatrix[1].y = ry*ry * (1.0f - c) + c;
	rotMatrix[2].y = ry*rz * (1.0f - c) - rx * s;
	rotMatrix[3].y = 0.0f;

	rotMatrix[0].z = rz*rx * (1.0f - c) - ry * s;
	rotMatrix[1].z = rz*ry * (1.0f - c) + rx * s;
	rotMatrix[2].z = rz*rz * (1.0f - c) + c;
	rotMatrix[3].z = 0.0f;

	rotMatrix[0].w = 0.0f;
	rotMatrix[1].w = 0.0f;
	rotMatrix[2].w = 0.0f;
	rotMatrix[3].w = 1.0f;
	matrix4x4_multiply(calcMatrix, calcMatrix2, rotMatrix);

	// translation
	rotMatrix[0].x = 1.0f;
	rotMatrix[1].x = 0.0f;
	rotMatrix[2].x = 0.0f;
	rotMatrix[3].x = translation->x;

	rotMatrix[0].y = 0.0f;
	rotMatrix[1].y = 1.0f;
	rotMatrix[2].y = 0.0f;
	rotMatrix[3].y = translation->y;

	rotMatrix[0].z = 0.0f;
	rotMatrix[1].z = 0.0f;
	rotMatrix[2].z = 1.0f;
	rotMatrix[3].z = translation->z;

	rotMatrix[0].w = 0.0f;
	rotMatrix[1].w = 0.0f;
	rotMatrix[2].w = 0.0f;
	rotMatrix[3].w = 1.0f;

	matrix4x4_multiply(matrixOut, calcMatrix, rotMatrix);	
}

void vector_multiplyMatrix(vector3D_t* vector, vector4D_t matrix[4])
{
	vector4D_t v;
	v.x = vector->x;
	v.y = vector->y;
	v.z = vector->z;
	v.w = 1.0f;
	// multiply x
	vector->x = v.x * matrix[0].x + v.y * matrix[1].x + v.z * matrix[2].x + v.w * matrix[3].x;
	vector->y = v.x * matrix[0].y + v.y * matrix[1].y + v.z * matrix[2].y + v.w * matrix[3].y;
	vector->z = v.x * matrix[0].z + v.y * matrix[1].z + v.z * matrix[2].z + v.w * matrix[3].z;
	float w = v.x * matrix[0].w + v.y * matrix[1].w + v.z * matrix[2].w + v.w * matrix[3].w;
	w = 1.0f / w;
	vector->x *= w;
	vector->y *= w;
	vector->z *= w;
}

void vector_normalize(vector3D_t *v)
{
	float l = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
	l = 1.0f / l;
	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void vector_normalize(vector2D_t *v)
{
	float l = sqrt(v->x*v->x + v->y*v->y);
	l = 1.0f / l;
	v->x *= l;
	v->y *= l;
}

float vector_length(vector2D_t *v)
{
	return sqrt(v->x*v->x + v->y*v->y);
}

float vector_length(vector3D_t *v)
{
	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
}

void vector_copy(vector2D_t *dst, vector2D_t *src)
{
	dst->x = src->x;
	dst->y = src->y;
}

void vector_copy(vector3D_t *dst, vector3D_t *src)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}


void vector_sub(vector2D_t *dst, vector2D_t *a, vector2D_t *b)
{
	dst->x = a->x - b->x;
	dst->y = a->y - b->y;
}

void vector_sub(vector3D_t *dst, vector3D_t *a, vector3D_t *b)
{
	dst->x = a->x - b->x;
	dst->y = a->y - b->y;
	dst->z = a->z - b->z;
}

void vector_rotate(vector2D_t *dst, vector2D_t *a, float angle)
{
	//x' = cos(a) * x - sin(a) * y
	//y' = sin(a) * x + cos(a) * y

	float ca = cos(angle);
	float sa = sin(angle);

	dst->x = ca * a->x - sa * a->y;
	dst->y = sa * a->x + ca * a->y;
}

float vector_distance(vector3D_t *v1, vector3D_t *v2)
{
	float dx = v1->x - v2->x;
	float dy = v1->y - v2->y;
	float dz = v1->z - v2->z;
	return sqrt(dx*dx + dy*dy + dz*dz);
}

float vector_distance(vector2D_t *v1, vector2D_t *v2)
{
	float dx = v1->x - v2->x;
	float dy = v1->y - v2->y;
	return sqrt(dx*dx + dy*dy);
}

float vector_distanceSquare(vector3D_t *v1, vector3D_t *v2)
{
	float dx = v1->x - v2->x;
	float dy = v1->y - v2->y;
	float dz = v1->z - v2->z;
	return dx*dx + dy*dy + dz*dz;
}

void vector_rotate(vector2D_t *v, float angle)
{
	float s = sinf(angle);
	float c = cosf(angle);

	float nx = c * v->x - s * v->y;
	float ny = s * v->x + c * v->y;

	v->x = nx;
	v->y = ny;
}

void vector_rotateXZ(vector3D_t *dst, vector3D_t *a, float angle)
{
	//x' = cos(a) * x - sin(a) * y
	//y' = sin(a) * x + cos(a) * y

	float ca = cos(angle);
	float sa = sin(angle);

	dst->x = ca * a->x - sa * a->z;
	dst->z = sa * a->x + ca * a->z;
	dst->y = a->y;
}

void vector_addMul(vector2D_t *dst, vector2D_t *a, vector2D_t *b, float factor)
{
	dst->x = a->x + b->x * factor;
	dst->y = a->y + b->y * factor;
}

void vector_scale(vector2D_t *dst, float factor)
{
	dst->x *= factor;
	dst->y *= factor;
}

void vector_scale(vector2D_t *dst, vector2D_t *a, float factor)
{
	dst->x = a->x * factor;
	dst->y = a->y * factor;
}

void vector_scale(vector3D_t *dst, vector3D_t *a, float factor)
{
	dst->x = a->x * factor;
	dst->y = a->y * factor;
	dst->z = a->z * factor;
}

void vector_cross(vector3D_t *dst, vector3D_t *a, vector3D_t *b)
{
	float xR = a->y * b->z - a->z * b->y;
	float yR = a->z * b->x - a->x * b->z;
	float zR = a->x * b->y - a->y * b->x;
	dst->x = xR;
	dst->y = yR;
	dst->z = zR;
}

float vector_dot(vector3D_t *a, vector3D_t *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
	
	/*float xR = a->y * b->z - a->z * b->y;
	float yR = a->z * b->x - a->x * b->z;
	float zR = a->x * b->y - a->y * b->x;
	dst->x = xR;
	dst->y = yR;
	dst->z = zR;*/
}

float vector_dot(vector2D_t *a, vector2D_t *b)
{
	return a->x * b->x + a->y * b->y;
}

void vector_calculateNormal(vector3D_t* normalOutput, vector3D_t* v1, vector3D_t* v2, vector3D_t* v3)
{
	//vector3D_t v0, v1, v2;
	//vector_copy(&v0, &progmeshTriangle->vertex[0]->position);
	//vector_copy(&v1, &progmeshTriangle->vertex[1]->position);
	//vector_copy(&v2, &progmeshTriangle->vertex[2]->position);

	vector3D_t t0, t1;
	t0.x = v2->x - v1->x;
	t0.y = v2->y - v1->y;
	t0.z = v2->z - v1->z;

	t1.x = v3->x - v2->x;
	t1.y = v3->y - v2->y;
	t1.z = v3->z - v2->z;

	//vector_normalize(&t0);
	//vector_normalize(&t1);

	vector_cross(normalOutput, &t0, &t1);
	vector_normalize(normalOutput);
}

void vector_calculateNormal(vector3D_t* normalOutput, vector3D_t* dif1, vector3D_t* dif2)
{
	vector3D_t t0, t1;

	vector_copy(&t0, dif1);
	vector_copy(&t1, dif2);

	vector_normalize(&t0);
	vector_normalize(&t1);

	vector_cross(normalOutput, &t0, &t1);
	vector_normalize(normalOutput);
}

float vector_distanceRayToPoint(vector3D_t* origin, vector3D_t* direction, vector3D_t* point)
{
	//vector3D_t c = Point - a;	// Vector from a to Point
	vector3D_t c;
	c.x = point->x - origin->x;
	c.y = point->y - origin->y;
	c.z = point->z - origin->z;
	//vector3D_t v = (b - a).Normalize();	// Unit Vector from a to b
	//float d = (b - a).Length();	// Length of the line segment
	float t = vector_dot(direction, &c);//v.DotProduct©;	// Intersection point Distance from a

	vector3D_t v;
	v.x = origin->x + t * direction->x;
	v.y = origin->y + t * direction->y;
	v.z = origin->z + t * direction->z;
	return vector_distance(point, &v);
}

float vector_projectPointOnRay(vector3D_t* origin, vector3D_t* direction, vector3D_t* point)
{
	vector3D_t c;
	c.x = point->x - origin->x;
	c.y = point->y - origin->y;
	c.z = point->z - origin->z;
	float t = vector_dot(direction, &c);
	return t;
}

float vector_projectPointOnRay(vector2D_t* origin, vector2D_t* direction, vector2D_t* point)
{
	vector2D_t c;
	c.x = point->x - origin->x;
	c.y = point->y - origin->y;
	float t = vector_dot(direction, &c);
	return t;
}

// random
//float rdF01seed = 1;
//float random_float01()
//{
//	seed *= 16807;
//	return ((float)seed) / (float)0x80000000;
//}

// generates a random float between 0 and 1
float random_float01()
{
	return ((float)rand()/(float)RAND_MAX);
}


/*
 * Intersections
 */
bool intersection_rayBox(vector3D_t* orig, vector3D_t* dir, vector3D_t* box_max, vector3D_t* box_min, float* distance)
{
	vector3D_t tmin;
	tmin.x = (box_min->x - orig->x)/dir->x;
	tmin.y = (box_min->y - orig->y)/dir->y;
	tmin.z = (box_min->z - orig->z)/dir->z;
	vector3D_t tmax;
	tmax.x = (box_max->x - orig->x)/dir->x;
	tmax.y = (box_max->y - orig->y)/dir->y;
	tmax.z = (box_max->z - orig->z)/dir->z;

	vector3D_t real_min;
	real_min.x = min(tmin.x, tmax.x);
	real_min.y = min(tmin.y, tmax.y);
	real_min.z = min(tmin.z, tmax.z);
	vector3D_t real_max;
	real_max.x = max(tmin.x, tmax.x);
	real_max.y = max(tmin.y, tmax.y);
	real_max.z = max(tmin.z, tmax.z);

	float minmax = min( min(real_max.x, real_max.y), real_max.z);
	float maxmin = max( max(real_min.x, real_min.y), real_min.z);

	*distance = maxmin;
	return (minmax >= maxmin);
}

/*
 * intersectionFace is the face that intersects the ray
 * Return values: top(0), bottom(1), left(2), right(3), front(4), back(5)
 */
bool intersection_rayBox(vector3D_t* orig, vector3D_t* dir, vector3D_t* box_max, vector3D_t* box_min, float* distance, uint8* intersectionFace)
{
	vector3D_t tmin;
	tmin.x = (box_min->x - orig->x)/dir->x;
	tmin.y = (box_min->y - orig->y)/dir->y;
	tmin.z = (box_min->z - orig->z)/dir->z;
	vector3D_t tmax;
	tmax.x = (box_max->x - orig->x)/dir->x;
	tmax.y = (box_max->y - orig->y)/dir->y;
	tmax.z = (box_max->z - orig->z)/dir->z;

	vector3D_t real_min;
	real_min.x = min(tmin.x, tmax.x);
	real_min.y = min(tmin.y, tmax.y);
	real_min.z = min(tmin.z, tmax.z);
	vector3D_t real_max;
	real_max.x = max(tmin.x, tmax.x);
	real_max.y = max(tmin.y, tmax.y);
	real_max.z = max(tmin.z, tmax.z);

	float minmax = min( min(real_max.x, real_max.y), real_max.z);
	float maxmin = max( max(real_min.x, real_min.y), real_min.z);

	*distance = maxmin;

	if( minmax < maxmin )
		return false;

	vector3D_t intersection;
	intersection.x = orig->x + dir->x * maxmin;
	intersection.y = orig->y + dir->y * maxmin;
	intersection.z = orig->z + dir->z * maxmin;

	// measure distance to the 6 planes:
	//float planeDistance;
	// top
	if( _fastAbs(intersection.y - box_max->y) <= 0.001f )
		*intersectionFace = 0;
	// bottom
	else if( _fastAbs(intersection.y - box_min->y) <= 0.001f )
		*intersectionFace = 1;
	// left
	else if( _fastAbs(intersection.x - box_min->x) <= 0.001f )
		*intersectionFace = 2;
	// right
	else if( _fastAbs(intersection.x - box_max->x) <= 0.001f )
		*intersectionFace = 3;
	// front
	else if( _fastAbs(intersection.z - box_min->z) <= 0.001f )
		*intersectionFace = 4;
	// back
	else
		*intersectionFace = 5;

	return true;
}


// fast abs
float _fastAbs(float v)
{
	if( v < 0.0f )
		return -v;
	return v;
}

/*
 * Returns the index of the least set bit (0-31)
 * If no bit is set at all, returns 32
 */
uint32 getLeastBitIndex(uint32 mask)
{
	// binary search using a shifted AND mask
	// always 5 iterations
	uint32 cIndex = 16;
	if( mask == 0 )
		return 32; // no bit set
	// iteration1
	if( mask&(0xFFFFFFFF>>(32-cIndex)) )
		cIndex -= 8;
	else
		cIndex += 8;
	// iteration2
	if( mask&(0xFFFFFFFF>>(32-cIndex)) )
		cIndex -= 4;
	else
		cIndex += 4;
	// iteration3
	if( mask&(0xFFFFFFFF>>(32-cIndex)) )
		cIndex -= 2;
	else
		cIndex += 2;
	// iteration4
	if( mask&(0xFFFFFFFF>>(32-cIndex)) )
		cIndex -= 1;
	else
		cIndex += 1;
	// iteration5 (fix)
	if( mask&(0xFFFFFFFF>>(32-cIndex)) )
		cIndex -= 1;
	return cIndex;
}
