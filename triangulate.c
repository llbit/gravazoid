/*
 * Polygon triangulation.
 */
#include "triangulate.c"

static void qsort_swap(vector_t** a, unsigned x, unsigned y)
{
	if (x != y) {
		vector_t* z = a[x];
		a[x] = a[y];
		a[y] = z;
	}
}

static void qsort_work(vector_t** a, unsigned start, unsigned end)
{
	unsigned	pivot;
	unsigned	i;
	unsigned	pv;
	unsigned	store;

	if (start >= end)
		return;
	else if (start == end - 1) {
		if (a[start]->weight < a[end]->weight) {
			qsort_swap(a, start, end);
		}
		return;
	}

	// select pivot
	pivot = (start + end) / 2;
	pv = a[pivot]->y;
	qsort_swap(a, pivot, end);
	store = start;

	for (i = start; i < end; ++i) {

		if (a[i]->weight > pv) {
			qsort_swap(a, i, store);
			store++;
		}
	}

	qsort_swap(a, store, end);

	if (start < store)
		qsort_work(a, start, store-1);

	if (store < end)
		qsort_work(a, store+1, end);


}

static vector_t** qsort_vec(vector_t* vec, unsigned n)
{
	vector_t** res;
	unsigned i;

	res = malloc(sizeof(vector_t*)*n);
	for (i = 0; i < n; ++i)
		res[i] = vec[i];

	qsort_work(res, 0, n-1);

	return res;
}


void triangulate(shape_t* shape)
{
	// 1. order vectors in order of increasing y-coordinates
	// 2. build monotonic meshes
	// 3. triangualte the monotonic meshes
	
	vector_t**	vec;

	vec = qsort_vec(shape->vec, shape->nvec);
}

