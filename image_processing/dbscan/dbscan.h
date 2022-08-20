#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define UNCLASSIFIED -1
#define NOISE -2

#define CORE_POINT 1
#define NOT_CORE_POINT 0

#define SUCCESS 0
#define FAILURE -3

typedef struct point_s point_t;
struct point_s
{
	double x, y, z;
	int cluster_id;		//
	bool core_point;
	double score;		//해당 포인트가 가지고 있던 score or weight 등의 정보가 있다면 저장시켜준다.
	int l, t, r, b;
	int reserved;		//다른 용도로 사용하기 위해 추가함. 
};

typedef struct node_s node_t;
struct node_s {
	unsigned int index;
	node_t *next;
};


typedef struct epsilon_neighbours_s epsilon_neighbours_t;

struct epsilon_neighbours_s {
	unsigned int num_members;
	node_t *head, *tail;
};

node_t *create_node(unsigned int index);

int append_at_end(
	unsigned int index,
	epsilon_neighbours_t *en);

epsilon_neighbours_t *get_epsilon_neighbours(
	unsigned int index,
	point_t *points,
	unsigned int num_points,
	double epsilon,
	double(*dist)(point_t *a, point_t *b));

void print_epsilon_neighbours(
	point_t *points,
	epsilon_neighbours_t *en);

void destroy_epsilon_neighbours(epsilon_neighbours_t *en);

int dbscan(
	point_t *points,
	unsigned int num_points,
	double epsilon,
	unsigned int minpts,
	double(*dist)(point_t *a, point_t *b));

int expand(
	unsigned int index,
	unsigned int cluster_id,
	point_t *points,
	unsigned int num_points,
	double epsilon,
	unsigned int minpts,
	double(*dist)(point_t *a, point_t *b));

int spread(
	unsigned int index,
	epsilon_neighbours_t *seeds,
	unsigned int cluster_id,
	point_t *points,
	unsigned int num_points,
	double epsilon,
	unsigned int minpts,
	double(*dist)(point_t *a, point_t *b));

double euclidean_dist(point_t *a, point_t *b);

double adjacent_intensity_dist(point_t *a, point_t *b);

unsigned int parse_input(
	FILE *file,
	point_t **points,
	double *epsilon,
	unsigned int *minpts);

void print_points(
	point_t *points,
	unsigned int num_points);
