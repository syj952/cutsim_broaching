// cuda_diff_volume.cu
#include <cuda_runtime.h>
#include <stdio.h>
#include <cmath>
///#include <vector>



struct GLVertex {
    float x, y, z;
};

struct CudaNodeData {
    float x[8];  // �ڵ㶥��� x ����
    float y[8];  // �ڵ㶥��� y ����
    float z[8];  // �ڵ㶥��� z ����
    float f[8];  // ÿ������ľ���ֵ
    int node_id[8];    // ����ȫ�ֽڵ�ID
};

// ���� VolumeParams �ṹ��
struct VolumeParams {

    int type;  // ʹ����������ö�٣��������Ͳ�ƥ������

    static const int SPHERE_VOLUME = 0;
    static const int CYLINDER_VOLUME = 1;
    static const int RECTANGLE_VOLUME = 2;

    // Ϊÿ�������ṹ��������������
    struct SphereParams {
        float x, y, z;
        float radius;
        float r, g, b;
    };

    struct CylinderParams {
        float x, y, z;
        float radius;
        float length;
        float angle_x, angle_y, angle_z;
        float r, g, b;
        double holderradius;
    };
    struct RectangleParams {
        float center_x, center_y, center_z;
        float length_x, length_y, length_z;
        float r, g, b;
    };

    union {
        SphereParams sphere;
        CylinderParams cylinder;
        RectangleParams rectangle;
    } params;
};

struct CutterSegment {
    int type;
    double radius1, radius2, length;
    GLVertex center, angle;
    double z_start, z_end;

    // 添加默认构造函数
    CutterSegment() : type(0), radius1(0.0), radius2(0.0), length(0.0), center(), z_start(0.0), z_end(0.0) {}

    // 添加带参数的构造函数
    CutterSegment(int t, double r1, double r2, double len, double zs, double ze)
        : type(t), radius1(r1), radius2(r2), length(len), center(), z_start(zs), z_end(ze) {
    }
};



struct broaching_BladeParams {
    float center_x, center_y, center_z;
    float dx, dy, dz;
    float cube_resolution_1;
    double* point_r_blade;  // �豸ָ��
    int point_r_count;      // �������
    GLVertex* blade_points;  // ��ǰ�����еĵ�����ָ��
    int blade_points_count;  // ��ǰ�����еĵ�����
    int blade_id;           // ��ǰ������������ID
    int device_id;

    // ǰ����ƽ����Ϣ
    int plane_count;        // ƽ������
    GLVertex* plane_normals;  // ƽ�淨����
    GLVertex* plane_points;   // ƽ���ϵĵ�
    // ������Ը�����Ҫ���Ӹ������
};

struct milling_BladeParams {
    float center_x, center_y, center_z;
    float dx, dy, dz;
    float cube_resolution;
    double* point_r_blade;  // �豸ָ��
    int point_r_count;      // �������
    GLVertex* blade_points;  // ��ΪGLVertex����ָ��
    int blade_points_count;  // �������
    int device_id;
    // ������Ը�����Ҫ���Ӹ������
};

// STL 体积参数结构体
struct StlParams {
    GLVertex* facets_v1;
    GLVertex* facets_v2;
    GLVertex* facets_v3;
    GLVertex* facets_normal;
    GLVertex* V21;
    GLVertex* V21invV21dotV21;
    GLVertex* V32;
    GLVertex* V32invV32dotV32;
    GLVertex* V13;
    GLVertex* V13invV13dotV13;
    int facet_count;
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    float inv_cube_size;
    float r, g, b;
};

#define TOLERANCE 1e-5f
#define CALC_TOLERANCE 1e-6f
#define FLT_MAX 3.402823466e+38F

enum StlSide { INSIDE, OUTSIDE, UNDECIDED };
enum Property { ON_VERTEX = 0x4, ON_EDGE = 0x8, ON_INNER = 0x10 };

struct CANDIDATE {
    int index;
    StlSide side;
    float abs_d;
    float3 q;
    int property;
};

__device__ float3 facet_center(GLVertex* v1, GLVertex* v2, GLVertex* v3) {
    return make_float3(
        (v1->x + v2->x + v3->x) / 3.0f,
        (v1->y + v2->y + v3->y) / 3.0f,
        (v1->z + v2->z + v3->z) / 3.0f
    );
}

__device__ float3 normalize_float3(float3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-12f) {
        return make_float3(v.x / len, v.y / len, v.z / len);
    }
    return make_float3(0.0f, 0.0f, 0.0f);
}

__device__ float3 cross_float3(float3 a, float3 b) {
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

__device__ float dot_float3(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ float3 operator-(float3 a, float3 b) {
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__device__ float3 operator+(float3 a, float3 b) {
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__device__ float3 operator*(float3 a, float s) {
    return make_float3(a.x * s, a.y * s, a.z * s);
}

__device__ float3 operator*(float s, float3 a) {
    return make_float3(a.x * s, a.y * s, a.z * s);
}

__device__ float norm_float3(float3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

__device__ float norm_float3(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

// 设备函数：计算点到线段的距离
__device__ float distance_to_segment(float3 p, float3 a, float3 b) {
    float3 ab = make_float3(b.x - a.x, b.y - a.y, b.z - a.z);
    float3 ap = make_float3(p.x - a.x, p.y - a.y, p.z - a.z);
    float t = ap.x * ab.x + ap.y * ab.y + ap.z * ab.z;
    float len_sq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    if (len_sq > 1e-12f) {
        t /= len_sq;
    }
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float3 projection = make_float3(a.x + t * ab.x, a.y + t * ab.y, a.z + t * ab.z);
    float3 diff = make_float3(p.x - projection.x, p.y - projection.y, p.z - projection.z);
    return sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

// 设备函数：计算点到单个三角面的有向距离，与 volume.cpp 中 StlVolume::distance 保持一致
__device__ void distance_to_triangle_with_info(float3 p, GLVertex* v1, GLVertex* v2, GLVertex* v3, GLVertex* normal,
    GLVertex* V21_vec, GLVertex* V21inv_vec, GLVertex* V32_vec, GLVertex* V32inv_vec,
    GLVertex* V13_vec, GLVertex* V13inv_vec, bool* is_on_face, float3* q_out, int* property_out, StlSide* side_out, float* abs_d_out) {
    float3 p3 = make_float3(p.x, p.y, p.z);
    float3 v13 = make_float3(v1->x, v1->y, v1->z);
    float3 v23 = make_float3(v2->x, v2->y, v2->z);
    float3 v33 = make_float3(v3->x, v3->y, v3->z);
    float3 n = make_float3(normal->x, normal->y, normal->z);
    float3 V213 = make_float3(V21_vec->x, V21_vec->y, V21_vec->z);
    float3 V21inv3 = make_float3(V21inv_vec->x, V21inv_vec->y, V21inv_vec->z);

    float u = (p3.x - v13.x) * V21inv3.x + (p3.y - v13.y) * V21inv3.y + (p3.z - v13.z) * V21inv3.z;
    float3 q = make_float3(v13.x + u * V213.x, v13.y + u * V213.y, v13.z + u * V213.z);
    float3 q_p = make_float3(q.x - p3.x, q.y - p3.y, q.z - p3.z);
    float dir = dot_float3(q_p, n);
    float3 r = make_float3(p3.x + n.x * dir, p3.y + n.y * dir, p3.z + n.z * dir);

    float3 r_v1 = r - v13;
    float3 v1_v3 = v13 - v33;
    float3 n1 = cross_float3(r_v1, v1_v3);

    float3 r_v2 = r - v23;
    float3 v2_v1 = v23 - v13;
    float3 n2 = cross_float3(r_v2, v2_v1);

    float3 r_v3 = r - v33;
    float3 v3_v2 = v33 - v23;
    float3 n3 = cross_float3(r_v3, v3_v2);

    float s12 = dot_float3(n1, n2);
    float s23 = dot_float3(n2, n3);
    float s31 = dot_float3(n3, n1);

    if (s12 > 0.0f && s23 > 0.0f && s31 > 0.0f) {
        *is_on_face = true;
        *q_out = q;
        *property_out = ON_INNER;
        *abs_d_out = fabsf(dir) - CALC_TOLERANCE;
        float d1 = norm_float3(v1->x - p.x, v1->y - p.y, v1->z - p.z);
        float d2 = norm_float3(v2->x - p.x, v2->y - p.y, v2->z - p.z);
        float d3 = norm_float3(v3->x - p.x, v3->y - p.y, v3->z - p.z);
        if (*abs_d_out < d1 && *abs_d_out < d2 && *abs_d_out < d3) {
            *side_out = (dir > 0.0f) ? INSIDE : OUTSIDE;
        } else {
            *side_out = UNDECIDED;
        }
        return;
    }

    *is_on_face = false;
    float3 V323 = make_float3(V32_vec->x, V32_vec->y, V32_vec->z);
    float3 V32inv3 = make_float3(V32inv_vec->x, V32inv_vec->y, V32inv_vec->z);
    float3 V133 = make_float3(V13_vec->x, V13_vec->y, V13_vec->z);
    float3 V13inv3 = make_float3(V13inv_vec->x, V13inv_vec->y, V13inv_vec->z);

    float3 q12, q13, q32;
    float abs_d12, abs_d13, abs_d32;
    int q12_property, q13_property, q32_property;

    float u12 = (p3.x - v13.x) * V21inv3.x + (p3.y - v13.y) * V21inv3.y + (p3.z - v13.z) * V21inv3.z;
    if (u12 <= 0.0f) {
        q12 = v13;
        q12_property = ON_VERTEX + 1;
    }
    else if (u12 >= 1.0f) {
        q12 = v23;
        q12_property = ON_VERTEX + 2;
    }
    else {
        q12 = v13 + V213 * u12;
        q12_property = ON_EDGE + 1;
    }
    abs_d12 = norm_float3(q12 - p3);

    float u13 = (p3.x - v33.x) * V13inv3.x + (p3.y - v33.y) * V13inv3.y + (p3.z - v33.z) * V13inv3.z;
    if (u13 <= 0.0f) {
        q13 = v33;
        q13_property = ON_VERTEX + 3;
    }
    else if (u13 >= 1.0f) {
        q13 = v13;
        q13_property = ON_VERTEX + 1;
    }
    else {
        q13 = v33 + V133 * u13;
        q13_property = ON_EDGE + 3;
    }
    abs_d13 = norm_float3(q13 - p3);

    float u32 = (p3.x - v23.x) * V32inv3.x + (p3.y - v23.y) * V32inv3.y + (p3.z - v23.z) * V32inv3.z;
    if (u32 <= 0.0f) {
        q32 = v23;
        q32_property = ON_VERTEX + 2;
    }
    else if (u32 >= 1.0f) {
        q32 = v33;
        q32_property = ON_VERTEX + 3;
    }
    else {
        q32 = v23 + V323 * u32;
        q32_property = ON_EDGE + 2;
    }
    abs_d32 = norm_float3(q32 - p3);

    float3 closest_q;
    float min_abs_d;
    int property;

    if ((abs_d12 <= abs_d13) && (abs_d12 <= abs_d32)) {
        closest_q = q12;
        min_abs_d = abs_d12;
        property = q12_property;
    }
    else if ((abs_d13 < abs_d12) && (abs_d13 <= abs_d32)) {
        closest_q = q13;
        min_abs_d = abs_d13;
        property = q13_property;
    }
    else {
        closest_q = q32;
        min_abs_d = abs_d32;
        property = q32_property;
    }

    *q_out = closest_q;
    *abs_d_out = min_abs_d;
    *property_out = property;

    float3 q_p_closest = closest_q - p3;
    float dir_sign = dot_float3(q_p_closest, n);
    *side_out = (dir_sign > 0.0f) ? INSIDE : OUTSIDE;
}

__device__ float distance_to_triangle(float3 p, GLVertex* v1, GLVertex* v2, GLVertex* v3, GLVertex* normal,
    GLVertex* V21_vec, GLVertex* V21inv_vec, GLVertex* V32_vec, GLVertex* V32inv_vec,
    GLVertex* V13_vec, GLVertex* V13inv_vec, bool* is_on_face, float3* q_out) {
    int property;
    StlSide side;
    float abs_d;
    distance_to_triangle_with_info(p, v1, v2, v3, normal, V21_vec, V21inv_vec, V32_vec, V32inv_vec,
        V13_vec, V13inv_vec, is_on_face, q_out, &property, &side, &abs_d);
    return (side == INSIDE) ? abs_d : -abs_d;
}

// 设备函数：计算点到 STL 体积的距离，带候选点机制和边界修正
__device__ float stl_distance_with_correction(float3 p, StlParams* stl) {
    CANDIDATE selected = { 0, UNDECIDED, FLT_MAX, make_float3(0,0,0), 0 };
    CANDIDATE second = { 0, UNDECIDED, FLT_MAX, make_float3(0,0,0), 0 };
    CANDIDATE third = { 0, UNDECIDED, FLT_MAX, make_float3(0,0,0), 0 };
    float min_dist = FLT_MAX;

    for (int i = 0; i < stl->facet_count; i++) {
        bool is_on_face;
        float3 q;
        int property;
        StlSide side;
        float abs_d;

        distance_to_triangle_with_info(p, &stl->facets_v1[i], &stl->facets_v2[i], &stl->facets_v3[i],
            &stl->facets_normal[i], &stl->V21[i], &stl->V21invV21dotV21[i],
            &stl->V32[i], &stl->V32invV32dotV32[i],
            &stl->V13[i], &stl->V13invV13dotV13[i],
            &is_on_face, &q, &property, &side, &abs_d);

        if (abs_d >= min_dist && abs_d > second.abs_d)
            continue;

        float dir = (side == INSIDE) ? abs_d : -abs_d;

        if (abs_d < min_dist) {
            min_dist = abs_d;
            third = second;
            second = selected;
            selected.index = i;
            selected.side = side;
            selected.abs_d = abs_d;
            selected.q = q;
            selected.property = property;
        }
        else {
            third = second;
            second.index = i;
            second.side = side;
            second.abs_d = abs_d;
            second.q = q;
            second.property = property;
        }
    }

    if (selected.side == UNDECIDED) {
        return -FLT_MAX;
    }

    float ret = (selected.side == INSIDE) ? selected.abs_d : -selected.abs_d;
    bool correction = (selected.property != ON_INNER);

    if (second.side != UNDECIDED && third.side != UNDECIDED) {
        if (norm_float3(selected.q - third.q) < norm_float3(selected.q - second.q)) {
            CANDIDATE temp = second;
            second = third;
            third = temp;
        }
    }

    if (correction) {
        if (((second.side != UNDECIDED) && (second.side != selected.side)) ||
            ((third.side != UNDECIDED) && (third.side != selected.side))) {

            if (norm_float3(selected.q - second.q) < CALC_TOLERANCE * 100.0f) {
                float3 fc1 = facet_center(&stl->facets_v1[selected.index], &stl->facets_v2[selected.index], &stl->facets_v3[selected.index]);
                float3 fc2 = facet_center(&stl->facets_v1[second.index], &stl->facets_v2[second.index], &stl->facets_v3[second.index]);

                float3 outer_vector = normalize_float3(normalize_float3(selected.q - fc1) + normalize_float3(selected.q - fc2));
                float3 normal_avg_vector = normalize_float3(
                    make_float3(stl->facets_normal[selected.index].x, stl->facets_normal[selected.index].y, stl->facets_normal[selected.index].z) +
                    make_float3(stl->facets_normal[second.index].x, stl->facets_normal[second.index].y, stl->facets_normal[second.index].z));

                if (third.side != UNDECIDED && norm_float3(selected.q - third.q) < CALC_TOLERANCE * 100.0f) {
                    float3 fc3 = facet_center(&stl->facets_v1[third.index], &stl->facets_v2[third.index], &stl->facets_v3[third.index]);
                    outer_vector = normalize_float3(outer_vector + normalize_float3(selected.q - fc3));
                    normal_avg_vector = normalize_float3(normal_avg_vector +
                        make_float3(stl->facets_normal[third.index].x, stl->facets_normal[third.index].y, stl->facets_normal[third.index].z));
                }

                if (dot_float3(normal_avg_vector, outer_vector) < 0.0f) {
                    outer_vector = outer_vector * -1.0f;
                }

                if (dot_float3(outer_vector, selected.q - p) < 0.0f) {
                    if (selected.side == INSIDE) {
                        selected.side = OUTSIDE;
                        ret = -selected.abs_d;
                    }
                }
                else {
                    if (selected.side == OUTSIDE) {
                        selected.side = INSIDE;
                        ret = selected.abs_d;
                    }
                }
            }
            else if ((second.side != selected.side) &&
                ((selected.property & ON_EDGE) || (second.property & ON_EDGE))) {

                bool do_correct = false;
                float3 edge1, edge2, p1, p2, edge_vec, point_vec;

                if ((selected.property & ~ON_EDGE) == 1) {
                    edge1 = make_float3(stl->V21[selected.index].x, stl->V21[selected.index].y, stl->V21[selected.index].z);
                    p1 = make_float3(stl->facets_v1[selected.index].x, stl->facets_v1[selected.index].y, stl->facets_v1[selected.index].z);
                }
                else if ((selected.property & ~ON_EDGE) == 2) {
                    edge1 = make_float3(stl->V32[selected.index].x, stl->V32[selected.index].y, stl->V32[selected.index].z);
                    p1 = make_float3(stl->facets_v2[selected.index].x, stl->facets_v2[selected.index].y, stl->facets_v2[selected.index].z);
                }
                else {
                    edge1 = make_float3(stl->V13[selected.index].x, stl->V13[selected.index].y, stl->V13[selected.index].z);
                    p1 = make_float3(stl->facets_v3[selected.index].x, stl->facets_v3[selected.index].y, stl->facets_v3[selected.index].z);
                }

                if ((second.property & ~ON_EDGE) == 1) {
                    edge2 = make_float3(stl->V21[second.index].x, stl->V21[second.index].y, stl->V21[second.index].z);
                    p2 = make_float3(stl->facets_v1[second.index].x, stl->facets_v1[second.index].y, stl->facets_v1[second.index].z);
                }
                else if ((second.property & ~ON_EDGE) == 2) {
                    edge2 = make_float3(stl->V32[second.index].x, stl->V32[second.index].y, stl->V32[second.index].z);
                    p2 = make_float3(stl->facets_v2[second.index].x, stl->facets_v2[second.index].y, stl->facets_v2[second.index].z);
                }
                else {
                    edge2 = make_float3(stl->V13[second.index].x, stl->V13[second.index].y, stl->V13[second.index].z);
                    p2 = make_float3(stl->facets_v3[second.index].x, stl->facets_v3[second.index].y, stl->facets_v3[second.index].z);
                }

                edge1 = normalize_float3(edge1);
                edge2 = normalize_float3(edge2);
                edge_vec = normalize_float3(p1 - p2);

                if (norm_float3(cross_float3(edge1, edge_vec)) < TOLERANCE && norm_float3(cross_float3(edge1, edge2)) < TOLERANCE) {
                    do_correct = true;
                }

                if (do_correct) {
                    float3 fc1 = facet_center(&stl->facets_v1[selected.index], &stl->facets_v2[selected.index], &stl->facets_v3[selected.index]);
                    float3 fc2 = facet_center(&stl->facets_v1[second.index], &stl->facets_v2[second.index], &stl->facets_v3[second.index]);
                    float3 outer_vector = normalize_float3(normalize_float3(selected.q - fc1) + normalize_float3(selected.q - fc2));
                    float3 normal_avg_vector = normalize_float3(
                        make_float3(stl->facets_normal[selected.index].x, stl->facets_normal[selected.index].y, stl->facets_normal[selected.index].z) +
                        make_float3(stl->facets_normal[second.index].x, stl->facets_normal[second.index].y, stl->facets_normal[second.index].z));

                    if (dot_float3(normal_avg_vector, outer_vector) < 0.0f) {
                        outer_vector = outer_vector * -1.0f;
                    }

                    if (dot_float3(outer_vector, selected.q - p) < 0.0f) {
                        if (selected.side == INSIDE) {
                            selected.side = OUTSIDE;
                            ret = -selected.abs_d;
                        }
                    }
                    else {
                        if (selected.side == OUTSIDE) {
                            selected.side = INSIDE;
                            ret = selected.abs_d;
                        }
                    }
                }
            }
        }

        float3 fc_sel = facet_center(&stl->facets_v1[selected.index], &stl->facets_v2[selected.index], &stl->facets_v3[selected.index]);
        float3 v = normalize_float3(fc_sel - p);
        float distance = norm_float3(fc_sel - p);

        for (int i = 0; i < stl->facet_count; i++) {
            if (i == selected.index) continue;

            float3 n_i = make_float3(stl->facets_normal[i].x, stl->facets_normal[i].y, stl->facets_normal[i].z);
            float v_n = dot_float3(v, n_i);
            if (fabsf(v_n) < CALC_TOLERANCE) continue;

            float3 v1_i = make_float3(stl->facets_v1[i].x, stl->facets_v1[i].y, stl->facets_v1[i].z);
            float t = dot_float3(v1_i - p, n_i) / v_n;
            if (t < 0.0f) continue;

            float3 r = p + v * t;
            float3 V13_i = make_float3(stl->V13[i].x, stl->V13[i].y, stl->V13[i].z);
            float3 V21_i = make_float3(stl->V21[i].x, stl->V21[i].y, stl->V21[i].z);
            float3 V32_i = make_float3(stl->V32[i].x, stl->V32[i].y, stl->V32[i].z);

            float3 n1 = cross_float3(r - v1_i, V13_i);
            float3 n2 = cross_float3(r - make_float3(stl->facets_v2[i].x, stl->facets_v2[i].y, stl->facets_v2[i].z), V21_i);
            float3 n3 = cross_float3(r - make_float3(stl->facets_v3[i].x, stl->facets_v3[i].y, stl->facets_v3[i].z), V32_i);

            float s12 = dot_float3(n1, n2);
            float s23 = dot_float3(n2, n3);
            float s31 = dot_float3(n3, n1);

            if (s12 > 0.0f && s23 > 0.0f && s31 > 0.0f) {
                if (t < distance) {
                    distance = t;
                    if ((selected.side == INSIDE) && (v_n < 0.0f)) {
                        selected.side = OUTSIDE;
                        ret = -selected.abs_d;
                    }
                    else if ((selected.side == OUTSIDE) && (v_n > 0.0f)) {
                        selected.side = INSIDE;
                        ret = selected.abs_d;
                    }
                }
            }
        }
    }

    return ret;
}

// 设备函数：计算点到 STL 体积的距离（兼容旧接口）
__device__ float stl_distance(float3 p, StlParams* stl) {
    return stl_distance_with_correction(p, stl);
}

// STL 求和核函数
__global__ void sum_stl_kernel(CudaNodeData* nodes, int numNodes, StlParams* stl) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int totalVertices = numNodes * 8;
    if (idx < totalVertices) {
        int nodeIdx = idx / 8;
        int vertexIdx = idx % 8;
        float3 p = make_float3(nodes[nodeIdx].x[vertexIdx], nodes[nodeIdx].y[vertexIdx], nodes[nodeIdx].z[vertexIdx]);
        float current_dist = nodes[nodeIdx].f[vertexIdx];
        float new_dist = stl_distance(p, stl);
        if (new_dist > current_dist) {
            nodes[nodeIdx].f[vertexIdx] = new_dist;
        }
    }
}

namespace cutsim {

    // 绕x轴旋转点
    __device__ float3 rotate_x(float3 p, double angle) {
        float cos_theta = cos(static_cast<float>(angle));
        float sin_theta = sin(static_cast<float>(angle));
        float3 rotated;
        rotated.x = p.x;
        rotated.y = p.y * cos_theta - p.z * sin_theta;
        rotated.z = p.y * sin_theta + p.z * cos_theta;
        return rotated;
    }

    // 绕y轴旋转点
    __device__ float3 rotate_y(float3 p, double angle) {
        float cos_theta = cos(static_cast<float>(angle));
        float sin_theta = sin(static_cast<float>(angle));
        float3 rotated;
        rotated.x = p.x * cos_theta + p.z * sin_theta;
        rotated.y = p.y;
        rotated.z = -p.x * sin_theta + p.z * cos_theta;
        return rotated;
    }

    // 绕z轴旋转点
    __device__ float3   rotate_z(float3 p, double angle) {
        float cos_theta = cos(static_cast<float>(angle));
        float sin_theta = sin(static_cast<float>(angle));
        float3 rotated;
        rotated.x = p.x * cos_theta - p.y * sin_theta;
        rotated.y = p.x * sin_theta + p.y * cos_theta;
        rotated.z = p.z;
        return rotated;
    }

    // ����������ת��������
    __device__ float3 rotate_point(float3 p, float ax, float ay, float az) {
        // ��X����ת
        float y1 = p.y * cosf(ax) - p.z * sinf(ax);
        float z1 = p.y * sinf(ax) + p.z * cosf(ax);

        // ��Y����ת
        float x2 = p.x * cosf(ay) + z1 * sinf(ay);
        float z2 = -p.x * sinf(ay) + z1 * cosf(ay);

        // ��Z����ת
        float x3 = x2 * cosf(az) - y1 * sinf(az);
        float y3 = x2 * sinf(az) + y1 * cosf(az);

        return make_float3(x3, y3, z2);
    }

    // ����㵽Բ����ľ��루����ʵ�֣�
    __device__ float cylinder_dist(float3 p, VolumeParams::CylinderParams* cyl) {
        // ���Բ��������Ƿ���Ч
        if (cyl->radius <= 0.0f || cyl->length <= 0.0f) {
            return -1000.0f; // ����һ���ϴ�ĸ�ֵ����ʾ��Ч
        }

        float3 t;
        float d;

#ifdef MULTI_AXIS
        // ����ģʽ���Ƚ���p�����߽Ƕ���ת
        // ע�⣺�������cyl.center��Բ�������ĵ㣬cyl.angle_x��cyl.angle_z����ת�Ƕ�
        float3 rotated_p = rotate_point(make_float3(p.x - cyl->x, p.y - cyl->y, p.z - cyl->z),
            -cyl->angle_x, 0.0f, -cyl->angle_z);
        t = rotated_p;

        // ���㵽Բ�����ߵľ��루XYƽ���ϵľ��룩
        d = sqrtf(rotated_p.x * rotated_p.x + rotated_p.y * rotated_p.y);
#else
        // ����ģʽ��ֱ�Ӽ������λ��
        t = make_float3(p.x - cyl->x, p.y - cyl->y, p.z - cyl->z);

        // ���㵽Բ�����ߵľ��루XYƽ���ϵľ��룩
        d = sqrtf(t.x * t.x + t.y * t.y);
#endif

        // ��ֹ��ֵ���
        if (isnan(d) || isinf(d) || d > 1e6f) {
            return -1000.0f;
        }

        // ����CPU�汾�߼��жϵ��λ��
        if (t.z >= 0.0f) {
            // ����Բ�����Ϸ���ͬ�߶�
            return t.z > cyl->length ? cyl->holderradius - d : cyl->radius - d;  // ��ֵ��ʾ�ڲ�����ֵ��ʾ�ⲿ
        }
        else {
            // ����Բ�����·�
            if (d < cyl->radius) {
                // ��Բ�����ڲ�
                return t.z;
            }
            else {
                // ��Բ����������
                // ���㵽�ײ���Ե�ľ���
                float3 n = make_float3(t.x, t.y, 0.0f);

                // ��ֹ������
                if (d < 1e-6f) {
                    return -1000.0f;
                }

                // ��һ�������ŵ��뾶����
                n.x = n.x * (cyl->radius / d);
                n.y = n.y * (cyl->radius / d);

                // ����㵽��Ե�ľ��벢ȡ��ֵ
                float3 diff = make_float3(t.x - n.x, t.y - n.y, t.z);
                float dist = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                // ���������Ƿ���Ч
                if (isnan(dist) || isinf(dist)) {
                    return -1000.0f;
                }

                return -dist;
            }
        }
    }
    // �������������㺯��
    __device__ float rectangle_dist(float3 p, VolumeParams::RectangleParams* rect) {
        // ���㵽����ľ���
        float dx = fabsf(p.x - rect->center_x) - rect->length_x / 2;
        float dy = fabsf(p.y - rect->center_y) - rect->length_y / 2;
        float dz = fabsf(p.z - rect->center_z) - rect->length_z / 2;

        // �����ڲ�/�ⲿ���ž���
        float inside_dx = fmaxf(dx, 0.0f);
        float inside_dy = fmaxf(dy, 0.0f);
        float inside_dz = fmaxf(dz, 0.0f);
        float dist = -sqrtf(inside_dx * inside_dx + inside_dy * inside_dy + inside_dz * inside_dz);

        // �ⲿ�������
        if (dx < 0 && dy < 0 && dz < 0) {
            return fminf(fminf(fabsf(dx), fabsf(dy)), fabsf(dz));
        }
        return dist;
    }
    // ����������ͼ������
    __device__ float calculate_distance(float3 point, VolumeParams* volume) {
        if (volume->type == VolumeParams::CYLINDER_VOLUME) {
            return cylinder_dist(point, &(volume->params.cylinder));
        }
        else if (volume->type == VolumeParams::RECTANGLE_VOLUME) {
            return rectangle_dist(point, &(volume->params.rectangle));
        }
        // �ɼ������������������
        return 10000.0f; // Ĭ�Ϸ��ؽϴ���ֵ��ʾ�ⲿ
    }


    // 计算点到刀具段的距离
    __device__ double calculateDistanceToSegment(float3 p, CutterSegment* seg) {
        // 调试输出：检查接收到的seg指针和z值

        // 计算相对于刀具中心的坐标
        float3 rel_p;
        rel_p.x = p.x - seg->center.x;
        rel_p.y = p.y - seg->center.y;
        rel_p.z = p.z - seg->center.z;

        // 先绕x轴旋转，再绕y轴旋转
        if (seg->angle.x != 0.0 || seg->angle.y != 0.0 || seg->angle.z != 0.0) {
            rel_p = rotate_x(rel_p, seg->angle.x);
            rel_p = rotate_y(rel_p, seg->angle.y);
            rel_p = rotate_z(rel_p, seg->angle.z);
        }

        // 计算到z轴的距离
        double d = sqrtf(rel_p.x * rel_p.x + rel_p.y * rel_p.y);

        // 计算相对于底面和顶面的坐标
        float3 tb;  // 相对于底面的坐标（底面在 z = seg->z_start）
        tb.x = rel_p.x;
        tb.y = rel_p.y;
        tb.z = rel_p.z - seg->z_start;

        float3 tt;  // 相对于顶面的坐标（顶面在 z = seg->z_end）
        tt.x = rel_p.x;
        tt.y = rel_p.y;
        tt.z = rel_p.z - seg->z_end;

        double radius = seg->radius1;  // 对于圆柱体，底面和顶面半径相同

        // 处理点在底面下方的情况 (rel_p.z < seg->z_start)
        if (rel_p.z < seg->z_start) {
            if (d < radius) {
                // 点在底面投影圆内：距离底面的距离
                return -tb.z;  // ✅ 修正：应该是 tb.z，不是 -tb.z
            }
            else {
                // 点在底面投影圆外：距离底面边缘最近

                float3 n;
                n.x = tb.x * (radius / d);
                n.y = tb.y * (radius / d);
                n.z = 0.0f;

                float3 diff;
                diff.x = tb.x - n.x;
                diff.y = tb.y - n.y;
                diff.z = tb.z;

                float dist = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                return dist;  // ✅ 修正：应该是 -dist，不是 dist
            }
        }

        // 处理点在顶面上方的情况 (rel_p.z > seg->z_end)
        if (rel_p.z > seg->z_end) {
            if (d < radius) {
                // 点在顶面投影圆内：距离顶面的距离
                return tt.z;  // ✅ 修正：应该是 -tt.z，不是 tt.z
            }
            else {
                // 点在顶面投影圆外：距离顶面边缘最近

                float3 n;
                n.x = tt.x * (radius / d);
                n.y = tt.y * (radius / d);
                n.z = 0.0f;

                float3 diff;
                diff.x = tt.x - n.x;
                diff.y = tt.y - n.y;
                diff.z = tt.z;

                float dist = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);


                return dist;  // ✅ 修正：应该是 -dist，不是 dist
            }
        }

        // 点在 z 范围内，根据刀具类型计算距离
        int seg_type = seg->type;
        switch (seg_type) {
        case 0: {  // CutterSegment::CYLINDER
            // 圆柱体：距离 = 半径 - sqrt(x^2 + y^2)
            double distance_to_side = - seg->radius1 + d;  // ✅ 修正：应该是 radius - d，不是 -radius + d

            // 检查是否在底面和顶面之间的边界穿越区域
            if (distance_to_side<0) {
                // 边界穿越区域：比较到侧面、底面、顶面的距离，选择最小的
                double distance_to_bottom = tb.z;
                double distance_to_top = -tt.z;

                // ✅ 完全按照 CylinderVolume::dist() 的逻辑
                if ((-distance_to_side < distance_to_bottom) && (-distance_to_side < distance_to_top)) {
                    return distance_to_side;
                }
                else {
                    return (distance_to_bottom < distance_to_top) ? -distance_to_bottom : -distance_to_top;
                }
            }

            return distance_to_side;
        }
        case 1: {  // CutterSegment::BALL
            // 球体：距离 = 半径 - sqrt(x^2 + y^2 + z^2)
            double dist = sqrtf(rel_p.x * rel_p.x + rel_p.y * rel_p.y + rel_p.z * rel_p.z);
            double result = -seg->radius1 + dist;
            return result;
        }
        case 2: {  // CutterSegment::CONE
            // 圆锥体：根据z坐标计算半径，然后计算距离
            double r = seg->radius1 + (seg->radius2 - seg->radius1) * (rel_p.z - seg->z_start) / (seg->z_end - seg->z_start);
            double result = -r + d;
            return result;
        }
        default:
            return INFINITY; // 未知类型，返回极大值
        }
    }

    // CUDA�˺����������ֵ����
    // �޸�CUDA�˺��������Ӹ���ı߽���ʹ������
    __global__ void diff_kernel(CudaNodeData* nodes, int numNodes, VolumeParams* volume) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;

        int nodeIdx = idx / 8;
        int vertexIdx = idx % 8;

        if (nodeIdx >= numNodes) {
            return;
        }

        // ��ȡ�ڵ�Ķ�������
        float3 point;
        point.x = nodes[nodeIdx].x[vertexIdx];
        point.y = nodes[nodeIdx].y[vertexIdx];
        point.z = nodes[nodeIdx].z[vertexIdx];
        // ��������Ƿ���Ч
        if (isnan(point.x) || isnan(point.y) || isnan(point.z) ||
            isinf(point.x) || isinf(point.y) || isinf(point.z)) {
            printf("������false");
            return;
        }

        // ����㵽����ľ���
        float dist = calculate_distance(point, volume);
        // ���������Ƿ���Ч
        if (isnan(dist) || isinf(dist)) {
            printf("������false");
            return;
        }
        // ���½ڵ�ľ���ֵ
        nodes[nodeIdx].f[vertexIdx] = dist;
    }

    __global__ void diff_cut_kernel(CudaNodeData* nodes, int numNodes, CutterSegment* segments, int numSegments) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        int nodeIdx = idx / 8;
        int vertexIdx = idx % 8;

        if (nodeIdx >= numNodes) {
            return;
        }

        // 获取节点的坐标点
        float3 point;
        point.x = nodes[nodeIdx].x[vertexIdx];
        point.y = nodes[nodeIdx].y[vertexIdx];
        point.z = nodes[nodeIdx].z[vertexIdx];

        // 检查点是否有效
        if (isnan(point.x) || isnan(point.y) || isnan(point.z) ||
            isinf(point.x) || isinf(point.y) || isinf(point.z)) {
            return;
        }

        // 遍历所有刀具段
        double minDist = INFINITY;
        for (int segIdx = 0; segIdx < numSegments; segIdx++) {
            // 直接索引访问刀具段
            CutterSegment* seg = &segments[segIdx];
            double dist = calculateDistanceToSegment(point, seg);
            if (dist < minDist) {
                minDist = dist;
            }
        }

        // 更新节点距离
        nodes[nodeIdx].f[vertexIdx] = static_cast<float>(-minDist);
    }

    __device__ float3 calculatePlaneNormal(float3 a, float3 b, float3 c) {
        float3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
        float3 ac = { c.x - a.x, c.y - a.y, c.z - a.z };

        // �����˵õ�������
        float3 normal = {
            ab.y * ac.z - ab.z * ac.y,
            ab.z * ac.x - ab.x * ac.z,
            ab.x * ac.y - ab.y * ac.x
        };

        // ��һ��������
        float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length < 1e-6f) return make_float3(0, 0, 0);
        return { normal.x / length, normal.y / length, normal.z / length };
    }
    // ������������
    __device__ float length(float3 v) {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    // ������һ��
    __device__ float3 normalize(float3 v) {
        float len = length(v);
        if (len < 1e-6f) return v;
        return make_float3(v.x / len, v.y / len, v.z / len);
    }

    __device__ float dot3(float3 a, float3 b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    __device__ float3 cross3(float3 a, float3 b) {
        return make_float3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    //// ���Ӹ������������Ƿ�����������
    __device__ bool pointInTriangle3D(float3 p, float3 a, float3 b, float3 c) {
        // ����������ƽ�淨����
        float3 v0 = { b.x - a.x, b.y - a.y, b.z - a.z };
        float3 v1 = { c.x - a.x, c.y - a.y, c.z - a.z };
        float3 normal = cross3(v0, v1);

        // ����㵽ƽ���ͶӰ
        float t = dot3(normal, make_float3(a.x - p.x, a.y - p.y, a.z - p.z))
            / dot3(normal, normal);
        float3 q = { p.x + t * normal.x,
                     p.y + t * normal.y,
                     p.z + t * normal.z };  // ͶӰ��

        // ʹ��ͶӰ����������������
        float3 v2 = { q.x - a.x, q.y - a.y, q.z - a.z };
        float d00 = dot3(v0, v0);
        float d01 = dot3(v0, v1);
        float d11 = dot3(v1, v1);
        float d20 = dot3(v2, v0);
        float d21 = dot3(v2, v1);

        float denom = d00 * d11 - d01 * d01;
        if (fabsf(denom) < 1e-6f) return false;

        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        return (u >= -1e-6f) && (v >= -1e-6f) && (w >= -1e-6f);
    }

    // ���Ƿ���͹�������ڣ��������桢����Ͳ��棩
    __device__ bool broaching_pointInConvexPolyhedron(float3 p, GLVertex* base_vertices, int n, float3 translation, GLVertex* plane_normals, GLVertex* plane_points, int plane_count) {
        // Check whether p lies between a rake-face plane and the same plane
        // translated by the tool step. The input normal direction is not stable
        // across blade files, so accept either signed-distance orientation.
        bool between_swept_planes = false;
        const float plane_eps = 1e-6f;
        for (int i = 0; i < plane_count; ++i) {
            // ��ȡƽ�淨����
            float3 normal = make_float3(plane_normals[i].x, plane_normals[i].y, plane_normals[i].z);

            // ����㵽ԭʼƽ��ľ���
            float3 plane_point = make_float3(plane_points[i].x, plane_points[i].y, plane_points[i].z);
            float dist = dot3(normal, make_float3(p.x - plane_point.x, p.y - plane_point.y, p.z - plane_point.z));

            // ����㵽ƽ��ƽ��ľ���
            float3 plane_point_next = make_float3(plane_point.x + translation.x, plane_point.y + translation.y, plane_point.z + translation.z);
            float dist_next = dot3(normal, make_float3(p.x - plane_point_next.x, p.y - plane_point_next.y, p.z - plane_point_next.z));

            //        printf("p:%f,%f,%f\n",p.x,p.y,p.z);
            //        printf("normal:%f,%f,%f\n",normal.x,normal.y,normal.z);
            //        printf("plane_point:%f,%f,%f\n",plane_point.x,plane_point.y,plane_point.z);
            //        printf("translation:%f,%f,%f\n",translation.x,translation.y,translation.z);
            if ((dist >= -plane_eps && dist_next <= plane_eps) ||
                (dist <= plane_eps && dist_next >= -plane_eps)) {
                between_swept_planes = true;
                break;
            }
        }

        if (!between_swept_planes)
        {
            return false;
        }

        if (n < 3) {
            return false;
        }

        bool section_inside = false;
        const float section_eps = 1e-6f;

        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            float z1 = base_vertices[i].z;
            float z2 = base_vertices[j].z;
            float x1 = base_vertices[i].x;
            float x2 = base_vertices[j].x;

            float edge_dx = x2 - x1;
            float edge_dz = z2 - z1;
            float point_dx = p.x - x1;
            float point_dz = p.z - z1;
            float cross = edge_dx * point_dz - edge_dz * point_dx;
            if (fabsf(cross) <= section_eps &&
                p.x >= fminf(x1, x2) - section_eps && p.x <= fmaxf(x1, x2) + section_eps &&
                p.z >= fminf(z1, z2) - section_eps && p.z <= fmaxf(z1, z2) + section_eps) {
                return true;
            }

            if (fabsf(edge_dz) < section_eps) {
                continue;
            }

            if ((z1 > p.z) != (z2 > p.z)) {
                float t = (p.z - z1) / edge_dz;
                float x_cross = x1 + t * edge_dx;
                if (p.x <= x_cross + section_eps) {
                    section_inside = !section_inside;
                }
            }
        }

        return section_inside;
    }

    // �����Ӻ������жϵ��Ƿ��������壨�����������ı��ι��ɣ�
    __device__ bool milling_pointInConvexPolyhedron(float3 p, GLVertex* quad1, GLVertex* quad2) {
        // ���ı��ηֽ�Ϊ6�������棨ÿ���ı��ηֽ�Ϊ2�������Σ�
        float3 faces[12][3] = {
            // ����������1
            {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
             make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
             make_float3(quad1[1].x, quad1[1].y, quad1[1].z)},
             // ����������2
             {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
              make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
              make_float3(quad1[0].x, quad1[0].y, quad1[0].z)},
              // ����������1
              {make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
               make_float3(quad2[1].x, quad2[1].y, quad2[1].z),
               make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
               // ����������2
               {make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
                make_float3(quad2[2].x, quad2[2].y, quad2[2].z),
                make_float3(quad2[3].x, quad2[3].y, quad2[3].z)},
                // �����ı��ηֽ��������1
                {make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                 make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                 make_float3(quad2[1].x, quad2[1].y, quad2[1].z)},
                {make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                 make_float3(quad2[1].x, quad2[1].y, quad2[1].z),
                 make_float3(quad2[0].x, quad2[0].y, quad2[0].z)},
                 //2
                 {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                  make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                  make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
                 {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                  make_float3(quad2[2].x, quad2[2].y, quad2[2].z),
                  make_float3(quad2[1].x, quad2[1].y, quad2[1].z)},
                  //3
                  {make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                   make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                   make_float3(quad2[3].x, quad2[3].y, quad2[3].z)},
                  {make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                   make_float3(quad2[3].x, quad2[3].y, quad2[3].z),
                   make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
                   //4
                   {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                    make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                    make_float3(quad2[0].x, quad2[0].y, quad2[0].z)},
                   {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                    make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
                    make_float3(quad2[3].x, quad2[3].y, quad2[3].z)}
        };

        //    for(int i = 0; i < 13; i=i+2) {
        //        float3 normal_1 = calculatePlaneNormal(faces[i][0], faces[i][1], faces[i][2]);
        //        float3 normal_2 = calculatePlaneNormal(faces[i+1][0], faces[i+1][1], faces[i+1][2]);
        //        float3 normal_cha = {
        //            normal_1.y * normal_2.z - normal_1.z * normal_2.y,
        //            normal_1.z * normal_2.x - normal_1.x * normal_2.z,
        //            normal_1.x * normal_2.y - normal_1.y * normal_2.x
        //        };
        //        float3 normal_3 = make_float3(faces[i][0].x-faces[i+1][1].x, faces[i][0].y-faces[i+1][1].y, faces[i][0].z-faces[i+1][1].z);
        //        float dot_result = dot3(normal_cha, normal_3);
        //        if (dot_result>0){
        //            // �����������������εĶ���˳��
        //            float3 temp[2][3];
        //            // ����ԭʼ������
        //            temp[0][0] = faces[i][0];//3 0
        //            temp[0][1] = faces[i][1];//2 1
        //            temp[0][2] = faces[i][2];//1 2
        //            temp[1][0] = faces[i+1][0];//3 0
        //            temp[1][1] = faces[i+1][1];//1 2
        //            temp[1][2] = faces[i+1][2];//0 3

        //            // �������ж���˳��
        //            faces[i][0] = temp[0][1];   // 2 1
        //            faces[i][1] = temp[0][2];   // 1 2
        //            faces[i][2] = temp[1][2];   // 0 3

        //            faces[i+1][0] = temp[0][1]; // 2 1
        //            faces[i+1][1] = temp[1][2]; // 0 3
        //            faces[i+1][2] = temp[0][0]; // 3 0
        //        }
        //    }

        // �����Ƿ������в����ͬһ��
        for (int i = 0; i < 12; ++i) {
            float3 normal = calculatePlaneNormal(faces[i][0], faces[i][1], faces[i][2]);
            float3 vec_to_p = make_float3(p.x - faces[i][2].x, p.y - faces[i][2].y, p.z - faces[i][2].z);
            float dot_result = dot3(normal, vec_to_p);

            // ������������һ���������ͬ
            if (dot_result < 0) {
                //printf("���ڲ���֮��\n");
                return false;
            }
        }
        return true;
    }


    // �㵽�߶ε���С���루3D�Ż��棩
    __device__ float pointToSegmentDist3D(float3 p, float3 a, float3 b) {
        float3 ab = make_float3(b.x - a.x, b.y - a.y, b.z - a.z);
        float3 ap = make_float3(p.x - a.x, p.y - a.y, p.z - a.z);

        float t = dot3(ap, ab) / dot3(ab, ab);
        t = fmaxf(0.0f, fminf(1.0f, t));

        float3 projection = make_float3(a.x + t * ab.x, a.y + t * ab.y, a.z + t * ab.z);
        float3 delta = make_float3(p.x - projection.x, p.y - projection.y, p.z - projection.z);

        return sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    }


    // ��Ҫ������㺯�����޸İ棩
    __device__ float broaching_distance_to_extrusion(float3 p, GLVertex* polygon, int count, float3 translation, float cube_resolution_1) {
        float side_min_dist = 1e12; // ���ڼ�¼�������С����

        // 3. �ȴ������棨ÿ���������ı��Σ�
        for (int i = 0; i < count; ++i) {
            int j = (i + 1) % count;

            // �ı��ε��ĸ�����
            float3 a_bottom = make_float3(polygon[i].x, polygon[i].y, polygon[i].z);
            float3 b_bottom = make_float3(polygon[j].x, polygon[j].y, polygon[j].z);
            float3 a_top = make_float3(polygon[i].x + translation.x, polygon[i].y + translation.y, polygon[i].z + translation.z);
            float3 b_top = make_float3(polygon[j].x + translation.x, polygon[j].y + translation.y, polygon[j].z + translation.z);

            // �ֽ�Ϊ����������
            float3 tri1[3] = { a_bottom, b_bottom, a_top };
            float3 tri2[3] = { b_bottom, b_top, a_top };

            // ������һ��������
            float3 tri1_normal = calculatePlaneNormal(tri1[0], tri1[1], tri1[2]);
            float tri1_dist = dot3(tri1_normal, make_float3(p.x - tri1[0].x, p.y - tri1[0].y, p.z - tri1[0].z));

            if (pointInTriangle3D(p, tri1[0], tri1[1], tri1[2])) {
                side_min_dist = fminf(side_min_dist, fabsf(tri1_dist));
            }
            else {
                // �ߺͶ�����
                for (int k = 0; k < 3; ++k) {
                    int l = (k + 1) % 3;
                    float edge_dist = pointToSegmentDist3D(p, tri1[k], tri1[l]);
                    side_min_dist = fminf(side_min_dist, fabsf(edge_dist));

                    float dx = p.x - tri1[k].x;
                    float dy = p.y - tri1[k].y;
                    float dz = p.z - tri1[k].z;
                    float vertex_dist = sqrtf(dx * dx + dy * dy + dz * dz);
                    side_min_dist = fminf(side_min_dist, fabsf(vertex_dist));
                }
            }

            // �����ڶ��������Σ�ͬ����
            float3 tri2_normal = calculatePlaneNormal(tri2[0], tri2[1], tri2[2]);
            float tri2_dist = dot3(tri2_normal, make_float3(p.x - tri2[0].x, p.y - tri2[0].y, p.z - tri2[0].z));

            if (pointInTriangle3D(p, tri2[0], tri2[1], tri2[2])) {
                side_min_dist = fminf(side_min_dist, fabsf(tri2_dist));
            }
            else {
                for (int k = 0; k < 3; ++k) {
                    int l = (k + 1) % 3;
                    float edge_dist = pointToSegmentDist3D(p, tri2[k], tri2[l]);
                    side_min_dist = fminf(side_min_dist, fabsf(edge_dist));

                    float dx = p.x - tri2[k].x;
                    float dy = p.y - tri2[k].y;
                    float dz = p.z - tri2[k].z;
                    float vertex_dist = sqrtf(dx * dx + dy * dy + dz * dz);
                    side_min_dist = fminf(side_min_dist, fabsf(vertex_dist));
                }
            }
        }

        // ���������С����С����ֵ��ֱ�ӷ��ز�����С����
        //if (side_min_dist <= cube_resolution_1*4) {
        return side_min_dist;
        //}

    }

    __device__ float milling_distanceToHexahedron(float3 p, GLVertex* quad1, GLVertex* quad2) {
        float min_dist = 1e12f;

        float3 faces[12][3] = {
            // ����������1
            {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
             make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
             make_float3(quad1[1].x, quad1[1].y, quad1[1].z)},
             // ����������2
             {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
              make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
              make_float3(quad1[0].x, quad1[0].y, quad1[0].z)},
              // ����������1
              {make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
               make_float3(quad2[1].x, quad2[1].y, quad2[1].z),
               make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
               // ����������2
               {make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
                make_float3(quad2[2].x, quad2[2].y, quad2[2].z),
                make_float3(quad2[3].x, quad2[3].y, quad2[3].z)},
                // �����ı��ηֽ��������1
                {make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                 make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                 make_float3(quad2[1].x, quad2[1].y, quad2[1].z)},
                {make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                 make_float3(quad2[1].x, quad2[1].y, quad2[1].z),
                 make_float3(quad2[0].x, quad2[0].y, quad2[0].z)},
                 //2
                 {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                  make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                  make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
                 {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
                  make_float3(quad2[2].x, quad2[2].y, quad2[2].z),
                  make_float3(quad2[1].x, quad2[1].y, quad2[1].z)},
                  //3
                  {make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                   make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                   make_float3(quad2[3].x, quad2[3].y, quad2[3].z)},
                  {make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
                   make_float3(quad2[3].x, quad2[3].y, quad2[3].z),
                   make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
                   //4
                   {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                    make_float3(quad1[0].x, quad1[0].y, quad1[0].z),
                    make_float3(quad2[0].x, quad2[0].y, quad2[0].z)},
                   {make_float3(quad1[3].x, quad1[3].y, quad1[3].z),
                    make_float3(quad2[0].x, quad2[0].y, quad2[0].z),
                    make_float3(quad2[3].x, quad2[3].y, quad2[3].z)}
        };

        //    for(int i = 0; i < 11; i=i+2) {
        //        float3 normal_1 = calculatePlaneNormal(faces[i][0], faces[i][1], faces[i][2]);
        //        float3 normal_2 = calculatePlaneNormal(faces[i+1][0], faces[i+1][1], faces[i+1][2]);
        //        float3 normal_cha = {
        //            normal_1.y * normal_2.z - normal_1.z * normal_2.y,
        //            normal_1.z * normal_2.x - normal_1.x * normal_2.z,
        //            normal_1.x * normal_2.y - normal_1.y * normal_2.x
        //        };
        //        float3 normal_3 = make_float3(faces[i][0].x-faces[i+1][1].x, faces[i][0].y-faces[i+1][1].y, faces[i][0].z-faces[i+1][1].z);
        //        float dot_result = dot3(normal_cha, normal_3);
        //        if (dot_result>0){
        //            //printf("%f\n",dot_result);
        //            // �����������������εĶ���˳��
        //            float3 temp[2][3];
        //            // ����ԭʼ������
        //            temp[0][0] = faces[i][0];//3 0
        //            temp[0][1] = faces[i][1];//2 1
        //            temp[0][2] = faces[i][2];//1 2
        //            temp[1][0] = faces[i+1][0];//3 0
        //            temp[1][1] = faces[i+1][1];//1 2
        //            temp[1][2] = faces[i+1][2];//0 3

        //            // �������ж���˳��
        //            faces[i][0] = temp[0][1];   // 2 1
        //            faces[i][1] = temp[0][2];   // 1 2
        //            faces[i][2] = temp[1][2];   // 0 3

        //            faces[i+1][0] = temp[0][1]; // 2 1
        //            faces[i+1][1] = temp[1][2]; // 0 3
        //            faces[i+1][2] = temp[0][0]; // 3 0
        //        }
        //    }

        for (int i = 4; i < 9; i = i + 1) {

            float3 normal = calculatePlaneNormal(faces[i][0], faces[i][1], faces[i][2]);
            bool in_triangle = pointInTriangle3D(p, faces[i][0], faces[i][1], faces[i][2]);

            if (in_triangle) {
                // �����������ڲ���ֱ��ʹ�������
                float face_dist = fabsf(dot3(normal, make_float3(p.x - faces[i][0].x, p.y - faces[i][0].y, p.z - faces[i][0].z)));
                min_dist = fminf(min_dist, face_dist);
            }
            else {
                // ���������ε�������
                for (int j = 0; j < 3; ++j) {
                    int k = (j + 1) % 3;
                    float3 a = faces[i][j];
                    float3 b = faces[i][k];

                    // �����߶β���t
                    float3 ab = make_float3(b.x - a.x, b.y - a.y, b.z - a.z);
                    float3 ap = make_float3(p.x - a.x, p.y - a.y, p.z - a.z);
                    float t = dot3(ap, ab) / dot3(ab, ab);

                    if (t >= 0.0f && t <= 1.0f) {
                        // ͶӰ���߶��ڲ�
                        min_dist = fminf(min_dist, pointToSegmentDist3D(p, a, b));
                    }
                    else {
                        // ͶӰ���߶��ⲿ�����㵽�����˵�ľ���
                        float dist_a = length(make_float3(p.x - a.x, p.y - a.y, p.z - a.z));
                        float dist_b = length(make_float3(p.x - b.x, p.y - b.y, p.z - b.z));
                        min_dist = fminf(min_dist, fminf(dist_a, dist_b));
                    }
                }
            }
        }
        return min_dist;
    }

    __device__ float milling_distanceToCutEdge(float3 p, GLVertex* quad1, GLVertex* quad2) {
        float min_dist = 1e12f;

        float3 faces[2][3] = {
            {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
             make_float3(quad1[2].x, quad1[2].y, quad1[2].z),
             make_float3(quad2[2].x, quad2[2].y, quad2[2].z)},
            {make_float3(quad1[1].x, quad1[1].y, quad1[1].z),
             make_float3(quad2[2].x, quad2[2].y, quad2[2].z),
             make_float3(quad2[1].x, quad2[1].y, quad2[1].z)}
        };

        for (int i = 0; i < 2; i = i + 1) {

            float3 normal = calculatePlaneNormal(faces[i][0], faces[i][1], faces[i][2]);
            bool in_triangle = pointInTriangle3D(p, faces[i][0], faces[i][1], faces[i][2]);
            float face_dist = fabsf(dot3(normal, make_float3(p.x - faces[i][0].x, p.y - faces[i][0].y, p.z - faces[i][0].z)));

            if (in_triangle) {
                // �����������ڲ���ֱ��ʹ�������
                min_dist = fminf(min_dist, face_dist);
            }
            else {
                // ���������ε�������
                for (int j = 0; j < 3; ++j) {
                    int k = (j + 1) % 3;
                    float3 a = faces[i][j];
                    float3 b = faces[i][k];

                    // �����߶β���t
                    float3 ab = make_float3(b.x - a.x, b.y - a.y, b.z - a.z);
                    float3 ap = make_float3(p.x - a.x, p.y - a.y, p.z - a.z);
                    float t = dot3(ap, ab) / dot3(ab, ab);

                    if (t >= 0.0f && t <= 1.0f) {
                        // ͶӰ���߶��ڲ�
                        min_dist = fminf(min_dist, pointToSegmentDist3D(p, a, b));
                    }
                    else {
                        // ͶӰ���߶��ⲿ�����㵽�����˵�ľ���
                        float dist_a = length(make_float3(p.x - a.x, p.y - a.y, p.z - a.z));
                        float dist_b = length(make_float3(p.x - b.x, p.y - b.y, p.z - b.z));
                        min_dist = fminf(min_dist, fminf(dist_a, dist_b));
                    }
                }
            }
        }
        return min_dist;
    }


    __global__ void blade_diff_kernel(
        CudaNodeData* nodes, int numNodes, broaching_BladeParams* blade,
        float* z_array, float* distence2edge, int* node_ids, int* record_count, int max_records)
    {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        int nodeIdx = idx / 8;
        int vertexIdx = idx % 8;

        if (nodeIdx >= numNodes) return;

        float x = nodes[nodeIdx].x[vertexIdx];
        float y = nodes[nodeIdx].y[vertexIdx];
        float z = nodes[nodeIdx].z[vertexIdx];
        //float f = nodes[nodeIdx].f[vertexIdx];
        float3 p = { x, y, z };

        // ����ƽ������
        float3 translation = { -blade->dx , -blade->dy , -blade->dz };

        // �жϵ��Ƿ����������ڣ����������棩
        bool inside = broaching_pointInConvexPolyhedron(p, blade->blade_points, blade->blade_points_count, translation, blade->plane_normals, blade->plane_points, blade->plane_count);

        GLVertex* polygon = (blade->blade_points_count > 0) ? blade->blade_points : nullptr;
        int count = blade->blade_points_count;

        // ��ʼ��������С�������������ʼ��Ϊ����ֵ��
        double min_dists[3] = { INFINITY, INFINITY, INFINITY };
        int min_is[3] = { -1, -1, -1 };

        for (int i = 0; i < count - 1; ++i) {
            double current_dist = sqrtf(
                (p.x - polygon[i].x) * (p.x - polygon[i].x) +
                (p.y - polygon[i].y) * (p.y - polygon[i].y) +
                (p.z - polygon[i].z) * (p.z - polygon[i].z)
            );
            float abs_dist = fabsf(current_dist);

            // ά��ǰ������Сֵ���������У�
            for (int j = 0; j < 3; ++j) {
                if (abs_dist < min_dists[j]) {
                    // ���ƽϴ��ֵ
                    for (int k = 2; k > j; --k) {
                        min_dists[k] = min_dists[k - 1];
                        min_is[k] = min_is[k - 1];
                    }
                    // ������ֵ
                    min_dists[j] = abs_dist;
                    min_is[j] = i;
                    break;
                }
            }
        }

        // ��¼������Сֵ��������飨������¼��δ��������ʱ��
        if (inside) {
            for (int j = 0; j < 3; ++j) {
                if (min_dists[j] != INFINITY) {  // ����¼��Ч����
                    int rec_idx = atomicAdd(record_count, 1);
                    if (rec_idx < max_records) {
                        z_array[rec_idx] = min_is[j];  // ע�⣺ԭ������z_array������Ҫ��������׼ȷ����edge_indices��
                        distence2edge[rec_idx] = min_dists[j];
                        node_ids[rec_idx] = nodes[nodeIdx].node_id[vertexIdx];
                    }
                }
            }
        }
        // ������С����
        float f_min_dist = broaching_distance_to_extrusion(p, blade->blade_points, blade->blade_points_count, translation, blade->cube_resolution_1);

        // ���·��ž���
        float signed_dist = inside ? fabsf(f_min_dist) : -fabsf(f_min_dist);
        nodes[nodeIdx].f[vertexIdx] = signed_dist;

    }

    // �޸ĺ��blade_diff_kernel
    __global__ void milling_blade_diff_kernel(
        CudaNodeData* nodes, int numNodes, milling_BladeParams* blade,
        float* z_array, float* distence2edge, int* node_ids, int* record_count, int max_records)
    {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        int nodeIdx = idx / 8;
        int vertexIdx = idx % 8;

        if (nodeIdx >= numNodes) return;

        float x = nodes[nodeIdx].x[vertexIdx];
        float y = nodes[nodeIdx].y[vertexIdx];
        float z = nodes[nodeIdx].z[vertexIdx];
        float3 p = { x, y, z };

        float f_min_dist = 1e12f;
        float cut_h;
        bool inside = false;

        // �������������ı��ζ�
        for (int i = 0; i + 8 <= blade->blade_points_count; i += 4) {
            GLVertex* quad1 = &blade->blade_points[i];
            GLVertex* quad2 = &blade->blade_points[i + 4];

            if (milling_pointInConvexPolyhedron(p, quad1, quad2)) {
                cut_h = milling_distanceToCutEdge(p, quad1, quad2);
                if (cut_h < blade->cube_resolution) {
                    f_min_dist = cut_h;
                }
                else  f_min_dist = fminf(f_min_dist, milling_distanceToHexahedron(p, quad1, quad2));
                inside = true;
                break;
            }
            else {
                f_min_dist = fminf(f_min_dist, milling_distanceToHexahedron(p, quad1, quad2));
            }

        }

        // ���ž������
        float signed_dist = inside ? f_min_dist : -f_min_dist;

        GLVertex* polygon = (blade->blade_points_count > 0) ? blade->blade_points : nullptr;
        int count = blade->blade_points_count;

        //    // ��ʼ����С����Ͷ�Ӧ�ı�����
        double min_dist = INFINITY;  // ��ʼ��Ϊ��󸡵���
        int min_i = -1;             // ��ʼ��Ϊ��Ч����

        for (int i = 0; i + 8 <= count; i += 4) {
            double current_dist = sqrtf(
                (p.x - polygon[i + 1].x) * (p.x - polygon[i + 1].x) +
                (p.y - polygon[i + 1].y) * (p.y - polygon[i + 1].y) +
                (p.z - polygon[i + 1].z) * (p.z - polygon[i + 1].z)
            );
            if (fabsf(current_dist) < min_dist) {
                min_dist = fabsf(current_dist);
                min_i = i / 4;  // ��¼��ǰ������
            }
        }
        int rec_idx = atomicAdd(record_count, 1);
        if (rec_idx < max_records) {
            z_array[rec_idx] = min_i;
            distence2edge[rec_idx] = min_dist;
            node_ids[rec_idx] = nodes[nodeIdx].node_id[vertexIdx];
        }
        // ���½ڵ����ֵ
        nodes[nodeIdx].f[vertexIdx] = signed_dist;
    }

    // ���ӿں�������C++�������
    extern "C" void cuda_diff_volume(CudaNodeData* host_nodes, int numNodes, VolumeParams host_volume) {
        // 添加调试输出
        //printf("CUDA函数开始执行，处理 %d 个节点\n", numNodes);
        //printf("host_nodes: %p\n", host_nodes);
        //printf("node_count: %d\n", numNodes);
        //fflush(stdout); // 强制刷新标准输出

        // 检查输入参数
        if (host_nodes == NULL || numNodes <= 0) {
            fprintf(stderr, "CUDA Error: 无效的输入参数\n");
            fflush(stderr);
            return;
        }

        // 检查节点数量是否过大
        size_t requiredMemory = numNodes * sizeof(CudaNodeData);
        size_t freeMemory, totalMemory;
        cudaMemGetInfo(&freeMemory, &totalMemory);
        //printf("需要的GPU内存: %zu 字节, 可用GPU内存: %zu 字节\n", requiredMemory, freeMemory);
        fflush(stdout);

        if (requiredMemory > freeMemory) {
            fprintf(stderr, "CUDA Error: GPU内存不足，需要 %zu 字节，但只有 %zu 字节可用\n",
                requiredMemory, freeMemory);
            fflush(stderr);
        }

        // 分配GPU内存
        CudaNodeData* dev_nodes = NULL;
        VolumeParams* dev_volume = NULL;

        cudaError_t err;

        // 分配设备内存
        err = cudaMalloc((void**)&dev_nodes, numNodes * sizeof(CudaNodeData));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data alloc): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            return;
        }
        //printf("成功分配设备内存 dev_nodes: %p\n", dev_nodes);
        //fflush(stdout);

        err = cudaMalloc((void**)&dev_volume, sizeof(VolumeParams));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (volume params alloc): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            return;
        }
        //printf("成功分配设备内存 dev_volume: %p\n", dev_volume);
        //fflush(stdout);

        // 拷贝数据到GPU
        err = cudaMemcpy(dev_nodes, host_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_volume);
            return;
        }
        //printf("成功拷贝节点数据到设备\n");
        //fflush(stdout);

        err = cudaMemcpy(dev_volume, &host_volume, sizeof(VolumeParams), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (volume params copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_volume);
            return;
        }
        //printf("成功拷贝体积参数到设备\n");
        //    //printf("体积类型: %d\n", host_volume.type);

        // 计算网格和块大小
        int threadsPerBlock = 512;
        int totalThreads = numNodes * 8;
        // CUDA 2080Ti gridDim.x 最大为 2,147,483,647，实际建议略小
        const int maxThreadsPerLaunch = 100'000'000; // 可根据实际情况调整
        int threadsProcessed = 0;

        while (threadsProcessed < totalThreads) {
            int threadsThisBatch = totalThreads - threadsProcessed;
            if (threadsThisBatch > maxThreadsPerLaunch) {
                threadsThisBatch = maxThreadsPerLaunch;
            }
            int grid = (threadsThisBatch + threadsPerBlock - 1) / threadsPerBlock;

            //printf("启动CUDA核函数:本批次%d个线程，gpu块分配：%d blocks, 每block %d threads\n", threadsThisBatch, grid, threadsPerBlock);
            //fflush(stdout);

            // kernel 内部索引加上偏移
            diff_kernel << <grid, threadsPerBlock >> > (
                reinterpret_cast<CudaNodeData*>((char*)dev_nodes + (threadsProcessed / 8) * sizeof(CudaNodeData)),
                threadsThisBatch / 8,
                dev_volume
                );

            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (kernel execution): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            err = cudaDeviceSynchronize();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (synchronize): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            threadsProcessed += threadsThisBatch;
        }


        //   printf("核函数执行完成\n");
        //  fflush(stdout);

        // 拷贝结果回CPU
        err = cudaMemcpy(host_nodes, dev_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (result copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_volume);
            return;
        }
        //printf("成功拷贝结果回主机\n");
        fflush(stdout);

        // 释放GPU内存
        cudaFree(dev_nodes);
        cudaFree(dev_volume);

        //printf("CUDA函数执行完成\n");
        //printf("\n");
        fflush(stdout);
    }

    extern "C" void broaching_cuda_diff_volume_blade(CudaNodeData* host_nodes, int numNodes, broaching_BladeParams host_blade,
        float** host_z_array, float** host_distence2edge, int** host_node_ids, int* host_record_count) {

        // ����������
        if (host_nodes == NULL || numNodes <= 0) {
            fprintf(stderr, "CUDA Error: ��Ч���������\n");
            fflush(stderr);
            return;
        }

        // ���ڵ������Ƿ����
        size_t requiredMemory = numNodes * sizeof(CudaNodeData);
        size_t freeMemory, totalMemory;
        cudaMemGetInfo(&freeMemory, &totalMemory);
        //printf("��Ҫ��GPU�ڴ�: %zu �ֽ�, ����GPU�ڴ�: %zu �ֽ�\n", requiredMemory, freeMemory);
        fflush(stdout);

        if (requiredMemory > freeMemory) {
            fprintf(stderr, "CUDA Error: GPU�ڴ治�㣬��Ҫ %zu �ֽڣ���ֻ�� %zu �ֽڿ���\n",
                requiredMemory, freeMemory);
            fflush(stderr);
        }

        // ����GPU�ڴ�
        CudaNodeData* dev_nodes = NULL;
        broaching_BladeParams* dev_volume = NULL;

        cudaError_t err;

        // �����豸�ڴ�
        cudaMalloc((void**)&dev_nodes, numNodes * sizeof(CudaNodeData));

        // ����blade_points����
        GLVertex* dev_blade_points = nullptr;
        GLVertex* dev_plane_normals = nullptr;
        GLVertex* dev_plane_points = nullptr;
        // ����blade_points�ڴ�
        if (host_blade.blade_points_count > 0) {
            err = cudaMalloc((void**)&dev_blade_points, host_blade.blade_points_count * sizeof(GLVertex));
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (blade points alloc): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                return;
            }
            // �������ݵ��豸
            err = cudaMemcpy(dev_blade_points, host_blade.blade_points,
                host_blade.blade_points_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (blade points copy): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                cudaFree(dev_blade_points);
                return;
            }
            host_blade.blade_points = dev_blade_points; // ����ָ��
        }

        // ����plane_normals��plane_points����
        if (host_blade.plane_count > 0) {
            // ����plane_normals�ڴ�
            err = cudaMalloc((void**)&dev_plane_normals, host_blade.plane_count * sizeof(GLVertex));
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (plane normals alloc): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                if (dev_blade_points) cudaFree(dev_blade_points);
                return;
            }
            // ����plane_normals���ݵ��豸
            err = cudaMemcpy(dev_plane_normals, host_blade.plane_normals,
                host_blade.plane_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (plane normals copy): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                if (dev_blade_points) cudaFree(dev_blade_points);
                cudaFree(dev_plane_normals);
                return;
            }
            host_blade.plane_normals = dev_plane_normals; // ����ָ��

            // ����plane_points�ڴ�
            err = cudaMalloc((void**)&dev_plane_points, host_blade.plane_count * sizeof(GLVertex));
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (plane points alloc): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                if (dev_blade_points) cudaFree(dev_blade_points);
                cudaFree(dev_plane_normals);
                return;
            }
            // ����plane_points���ݵ��豸
            err = cudaMemcpy(dev_plane_points, host_blade.plane_points,
                host_blade.plane_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (plane points copy): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                if (dev_blade_points) cudaFree(dev_blade_points);
                cudaFree(dev_plane_normals);
                cudaFree(dev_plane_points);
                return;
            }
            host_blade.plane_points = dev_plane_points; // ����ָ��
        }

        // 4. ����broaching_BladeParams�ڴ�
        err = cudaMalloc((void**)&dev_volume, sizeof(broaching_BladeParams));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (blade params alloc): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            if (dev_blade_points) cudaFree(dev_blade_points);
            return;
        }

        // 5. ����broaching_BladeParams���豸
        err = cudaMemcpy(dev_volume, &host_blade, sizeof(broaching_BladeParams), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (blade params copy): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            if (dev_blade_points) cudaFree(dev_blade_points);
            cudaFree(dev_volume);
            return;
        }

        // �������ݵ�GPU
        cudaMemcpy(dev_nodes, host_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyHostToDevice);


        // ��������Ϳ��С
        int threadsPerBlock = 256;
        int totalThreads = numNodes * 8;
        const int maxThreadsPerLaunch = 100'000'000; // �ɸ���ʵ���������
        int threadsProcessed = 0;

        //����z��d_min���鼰������
        float* dev_z_array = nullptr;
        float* dev_distence2edge = nullptr;
        int* dev_record_count = nullptr;
        int* dev_node_ids = nullptr;
        int max_records = numNodes * 8 * 3; // �����
        cudaMalloc(&dev_z_array, max_records * sizeof(float));
        cudaMalloc(&dev_distence2edge, max_records * sizeof(float));
        cudaMalloc(&dev_record_count, sizeof(int));
        cudaMemset(dev_record_count, 0, sizeof(int));
        cudaMalloc(&dev_node_ids, max_records * sizeof(int));

        while (threadsProcessed < totalThreads) {
            int threadsThisBatch = totalThreads - threadsProcessed;
            if (threadsThisBatch > maxThreadsPerLaunch) {
                threadsThisBatch = maxThreadsPerLaunch;
            }
            int grid = (threadsThisBatch + threadsPerBlock - 1) / threadsPerBlock;

            printf("����CUDA�˺���:������%d���̣߳�gpu����䣺%d blocks, ÿblock %d threads\n", threadsThisBatch, grid, threadsPerBlock);
            fflush(stdout);

            cudaSetDevice(host_blade.device_id);

            blade_diff_kernel << <grid, threadsPerBlock >> > (
                reinterpret_cast<CudaNodeData*>((char*)dev_nodes + (threadsProcessed / 8) * sizeof(CudaNodeData)),
                threadsThisBatch / 8,
                dev_volume,
                dev_z_array, dev_distence2edge, dev_node_ids, dev_record_count, max_records
                );


            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (kernel execution): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            err = cudaDeviceSynchronize();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (synchronize): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            threadsProcessed += threadsThisBatch;
        }

        // ������¼��
        cudaMemcpy(host_record_count, dev_record_count, sizeof(int), cudaMemcpyDeviceToHost);

        // ���������ڴ�
        *host_z_array = (float*)malloc((*host_record_count) * sizeof(float));
        *host_distence2edge = (float*)malloc((*host_record_count) * sizeof(float));
        *host_node_ids = (int*)malloc((*host_record_count) * sizeof(int));

        // ����z��d_min����,node_ids����
        cudaMemcpy(*host_z_array, dev_z_array, (*host_record_count) * sizeof(float), cudaMemcpyDeviceToHost);
        cudaMemcpy(*host_distence2edge, dev_distence2edge, (*host_record_count) * sizeof(float), cudaMemcpyDeviceToHost);
        cudaMemcpy(*host_node_ids, dev_node_ids, (*host_record_count) * sizeof(int), cudaMemcpyDeviceToHost);


        // �ͷ�
        cudaFree(dev_z_array);
        cudaFree(dev_distence2edge);
        cudaFree(dev_record_count);

        err = cudaMemcpy(host_nodes, dev_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (result copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_volume);
            return;
        }
        //printf("�ɹ��������������\n");
        fflush(stdout);

        // �ͷ�GPU�ڴ�
        cudaFree(dev_nodes);
        cudaFree(dev_volume);
        cudaFree(dev_blade_points);
        cudaFree(dev_node_ids);
        if (dev_plane_normals) cudaFree(dev_plane_normals);
        if (dev_plane_points) cudaFree(dev_plane_points);

        //printf("CUDA����ִ�����\n");
        //printf("\n");
        fflush(stdout);

    }

    extern "C" void milling_cuda_diff_volume_blade(CudaNodeData* host_nodes, int numNodes, milling_BladeParams host_blade,
        float** host_z_array, float** host_distence2edge, int** host_node_ids, int* host_record_count) {

        // ����������a
        if (host_nodes == NULL || numNodes <= 0) {
            fprintf(stderr, "CUDA Error: ��Ч���������\n");
            fflush(stderr);
            return;
        }

        // ���ڵ������Ƿ����
        size_t requiredMemory = numNodes * sizeof(CudaNodeData);
        size_t freeMemory, totalMemory;
        cudaMemGetInfo(&freeMemory, &totalMemory);
        //printf("��Ҫ��GPU�ڴ�: %zu �ֽ�, ����GPU�ڴ�: %zu �ֽ�\n", requiredMemory, freeMemory);
        fflush(stdout);

        if (requiredMemory > freeMemory) {
            fprintf(stderr, "CUDA Error: GPU�ڴ治�㣬��Ҫ %zu �ֽڣ���ֻ�� %zu �ֽڿ���\n",
                requiredMemory, freeMemory);
            fflush(stderr);
        }

        // ����GPU�ڴ�
        CudaNodeData* dev_nodes = NULL;
        milling_BladeParams* dev_volume = NULL;

        cudaError_t err;

        // �����豸�ڴ�
        err = cudaMalloc((void**)&dev_nodes, numNodes * sizeof(CudaNodeData));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data alloc): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            return;
        }

        // ����blade_points����
        GLVertex* dev_blade_points = nullptr;
        // ����blade_points�ڴ�
        if (host_blade.blade_points_count > 0) {
            err = cudaMalloc((void**)&dev_blade_points, host_blade.blade_points_count * sizeof(GLVertex));
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (blade points alloc): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                return;
            }
            // �������ݵ��豸
            err = cudaMemcpy(dev_blade_points, host_blade.blade_points,
                host_blade.blade_points_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (blade points copy): %s\n", cudaGetErrorString(err));
                cudaFree(dev_nodes);
                cudaFree(dev_blade_points);
                return;
            }
            host_blade.blade_points = dev_blade_points; // ����ָ��
        }

        // 4. ����BladeParams�ڴ�
        err = cudaMalloc((void**)&dev_volume, sizeof(milling_BladeParams));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (blade params alloc): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            if (dev_blade_points) cudaFree(dev_blade_points);
            return;
        }

        // 5. ����BladeParams���豸
        err = cudaMemcpy(dev_volume, &host_blade, sizeof(milling_BladeParams), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (blade params copy): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            if (dev_blade_points) cudaFree(dev_blade_points);
            cudaFree(dev_volume);
            return;
        }

        // �������ݵ�GPU
        cudaMemcpy(dev_nodes, host_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyHostToDevice);


        // ��������Ϳ��С
        int threadsPerBlock = 256;
        int totalThreads = numNodes * 8;
        const int maxThreadsPerLaunch = 100'000'000; // �ɸ���ʵ���������
        int threadsProcessed = 0;

        //����z��d_min���鼰������
        float* dev_z_array = nullptr;
        float* dev_distence2edge = nullptr;
        int* dev_record_count = nullptr;
        int* dev_node_ids = nullptr;
        int max_records = numNodes * 8; // �����
        cudaMalloc(&dev_z_array, max_records * sizeof(float));
        cudaMalloc(&dev_distence2edge, max_records * sizeof(float));
        cudaMalloc(&dev_record_count, sizeof(int));
        cudaMemset(dev_record_count, 0, sizeof(int));
        cudaMalloc(&dev_node_ids, max_records * sizeof(int));

        while (threadsProcessed < totalThreads) {
            int threadsThisBatch = totalThreads - threadsProcessed;
            if (threadsThisBatch > maxThreadsPerLaunch) {
                threadsThisBatch = maxThreadsPerLaunch;
            }
            int grid = (threadsThisBatch + threadsPerBlock - 1) / threadsPerBlock;

            printf("����CUDA�˺���:������%d���̣߳�gpu����䣺%d blocks, ÿblock %d threads\n", threadsThisBatch, grid, threadsPerBlock);
            fflush(stdout);

            cudaSetDevice(host_blade.device_id);

            milling_blade_diff_kernel << <grid, threadsPerBlock >> > (
                reinterpret_cast<CudaNodeData*>((char*)dev_nodes + (threadsProcessed / 8) * sizeof(CudaNodeData)),
                threadsThisBatch / 8,
                dev_volume,
                dev_z_array, dev_distence2edge, dev_node_ids, dev_record_count, max_records
                );


            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (kernel execution): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            err = cudaDeviceSynchronize();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (synchronize): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_volume);
                return;
            }

            threadsProcessed += threadsThisBatch;
        }

        // ������¼��
        cudaMemcpy(host_record_count, dev_record_count, sizeof(int), cudaMemcpyDeviceToHost);

        // ���������ڴ�
        *host_z_array = (float*)malloc((*host_record_count) * sizeof(float));
        *host_distence2edge = (float*)malloc((*host_record_count) * sizeof(float));
        *host_node_ids = (int*)malloc((*host_record_count) * sizeof(int));

        // ����z��d_min����,node_ids����
        cudaMemcpy(*host_z_array, dev_z_array, (*host_record_count) * sizeof(float), cudaMemcpyDeviceToHost);
        cudaMemcpy(*host_distence2edge, dev_distence2edge, (*host_record_count) * sizeof(float), cudaMemcpyDeviceToHost);
        cudaMemcpy(*host_node_ids, dev_node_ids, (*host_record_count) * sizeof(int), cudaMemcpyDeviceToHost);

        // �ͷ�
        cudaFree(dev_z_array);
        cudaFree(dev_distence2edge);
        cudaFree(dev_record_count);
        cudaFree(dev_node_ids);

        err = cudaMemcpy(host_nodes, dev_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (result copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_volume);
            return;
        }
        //printf("�ɹ��������������\n");
        fflush(stdout);

        // �ͷ�GPU�ڴ�
        if (dev_nodes != NULL) cudaFree(dev_nodes);
        if (dev_volume != NULL) cudaFree(dev_volume);
        if (dev_blade_points != NULL) cudaFree(dev_blade_points);

        //printf("CUDA����ִ�����\n");
        //printf("\n");
        fflush(stdout);

    }

    extern "C" void digitaltwin_milling_cuda_diff_volume_blade(CudaNodeData* host_nodes, int numNodes, CutterSegment* segments, int numSegments) {

        if (host_nodes == NULL || numNodes <= 0) {
            fprintf(stderr, "CUDA Error: ��Ч���������\n");
            fflush(stderr);
            return;
        }

        // ���ڵ������Ƿ����
        size_t requiredMemory = numNodes * sizeof(CudaNodeData);
        size_t freeMemory, totalMemory;
        cudaMemGetInfo(&freeMemory, &totalMemory);
        //printf("��Ҫ��GPU�ڴ�: %zu �ֽ�, ����GPU�ڴ�: %zu �ֽ�\n", requiredMemory, freeMemory);
        fflush(stdout);

        if (requiredMemory > freeMemory) {
            fprintf(stderr, "CUDA Error: GPU�ڴ治�㣬��Ҫ %zu �ֽڣ���ֻ�� %zu �ֽڿ���\n",
                requiredMemory, freeMemory);
            fflush(stderr);
        }

        // ����GPU�ڴ�
        CudaNodeData* dev_nodes = NULL;
        CutterSegment* dev_segments = NULL;

        cudaError_t err;

        // �����豸�ڴ�
        err = cudaMalloc((void**)&dev_nodes, numNodes * sizeof(CudaNodeData));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data alloc): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            return;
        }

        // 分配刀具段内存
        if (numSegments > 0) {
            err = cudaMalloc((void**)&dev_segments, numSegments * sizeof(CutterSegment));
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (segments alloc): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                return;
            }

            // 复制刀具段数据到设备内存
            err = cudaMemcpy(dev_segments, segments, numSegments * sizeof(CutterSegment), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (segments copy): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_segments);
                return;
            }
        }

        // �������ݵ�GPU
        err = cudaMemcpy(dev_nodes, host_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            if (dev_segments) cudaFree(dev_segments);
            return;
        }
        //printf("�ɹ������ڵ����ݵ��豸\n");
        //fflush(stdout);

        // 复制刀具段数据到设备内存
        if (numSegments > 0 && segments != NULL) {
            err = cudaMemcpy(dev_segments, segments, numSegments * sizeof(CutterSegment), cudaMemcpyHostToDevice);
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (segments copy): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                if (dev_segments) cudaFree(dev_segments);
                return;
            }
        }


        // ��������Ϳ��С
        int threadsPerBlock = 256;
        int totalThreads = numNodes * 8;
        // CUDA 2080Ti gridDim.x ���Ϊ 2,147,483,647��ʵ�ʽ�����С
        const int maxThreadsPerLaunch = 100'000'000; // �ɸ���ʵ���������
        int threadsProcessed = 0;

        while (threadsProcessed < totalThreads) {
            int threadsThisBatch = totalThreads - threadsProcessed;
            if (threadsThisBatch > maxThreadsPerLaunch) {
                threadsThisBatch = maxThreadsPerLaunch;
            }
            int grid = (threadsThisBatch + threadsPerBlock - 1) / threadsPerBlock;

            printf("����CUDA�˺���:������%d���̣߳�gpu����䣺%d blocks, ÿblock %d threads\n", threadsThisBatch, grid, threadsPerBlock);
            fflush(stdout);

            diff_cut_kernel << <grid, threadsPerBlock >> > (
                reinterpret_cast<CudaNodeData*>((char*)dev_nodes + (threadsProcessed / 8) * sizeof(CudaNodeData)),
                threadsThisBatch / 8,
                dev_segments,      // 直接传递刀具段指针
                numSegments
                );

            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (kernel execution): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_segments);
                return;
            }

            err = cudaDeviceSynchronize();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (synchronize): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_segments);
                return;
            }

            threadsProcessed += threadsThisBatch;
        }


        //   printf("�˺���ִ�����\n");
        //  fflush(stdout);

        // ���������CPU
        err = cudaMemcpy(host_nodes, dev_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (result copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_segments);
            return;
        }
        //printf("�ɹ��������������\n");
        fflush(stdout);

        // �ͷ�GPU�ڴ�
        cudaFree(dev_nodes);
        cudaFree(dev_segments);

        //printf("CUDA����ִ�����\n");
        //printf("\n");
        fflush(stdout);
    }

    // STL 求和 CUDA 函数
    extern "C" void cuda_sum_stl(CudaNodeData* host_nodes, int numNodes, StlParams host_stl) {
        // 检查输入参数
        if (host_nodes == NULL || numNodes <= 0) {
            fprintf(stderr, "CUDA Error: 无效的输入参数\n");
            fflush(stderr);
            return;
        }

        // 检查节点数量是否过大
        size_t requiredMemory = numNodes * sizeof(CudaNodeData);
        size_t freeMemory, totalMemory;
        cudaMemGetInfo(&freeMemory, &totalMemory);
        if (requiredMemory > freeMemory) {
            fprintf(stderr, "CUDA Error: GPU内存不足\n");
            fflush(stderr);
        }

        // 分配设备内存
        CudaNodeData* dev_nodes = NULL;
        StlParams* dev_stl = NULL;
        cudaError_t err;

        err = cudaMalloc((void**)&dev_nodes, numNodes * sizeof(CudaNodeData));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data alloc): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            return;
        }

        err = cudaMalloc((void**)&dev_stl, sizeof(StlParams));
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (stl params alloc): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            fflush(stderr);
            return;
        }

        // 拷贝节点数据到设备
        err = cudaMemcpy(dev_nodes, host_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (node data copy): %s\n", cudaGetErrorString(err));
            cudaFree(dev_nodes);
            cudaFree(dev_stl);
            fflush(stderr);
            return;
        }

        // 拷贝 STL 参数和分配设备内存
        GLVertex* d_facets_v1 = NULL;
        GLVertex* d_facets_v2 = NULL;
        GLVertex* d_facets_v3 = NULL;
        GLVertex* d_facets_normal = NULL;
        GLVertex* d_V21 = NULL;
        GLVertex* d_V21invV21dotV21 = NULL;
        GLVertex* d_V32 = NULL;
        GLVertex* d_V32invV32dotV32 = NULL;
        GLVertex* d_V13 = NULL;
        GLVertex* d_V13invV13dotV13 = NULL;

        int facet_count = host_stl.facet_count;

        cudaMalloc(&d_facets_v1, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_facets_v2, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_facets_v3, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_facets_normal, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V21, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V21invV21dotV21, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V32, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V32invV32dotV32, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V13, facet_count * sizeof(GLVertex));
        cudaMalloc(&d_V13invV13dotV13, facet_count * sizeof(GLVertex));

        cudaMemcpy(d_facets_v1, host_stl.facets_v1, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_v2, host_stl.facets_v2, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_v3, host_stl.facets_v3, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_normal, host_stl.facets_normal, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V21, host_stl.V21, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V21invV21dotV21, host_stl.V21invV21dotV21, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V32, host_stl.V32, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V32invV32dotV32, host_stl.V32invV32dotV32, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V13, host_stl.V13, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V13invV13dotV13, host_stl.V13invV13dotV13, facet_count * sizeof(GLVertex), cudaMemcpyHostToDevice);

        // 更新设备端 STL 参数中的指针
        StlParams dev_stl_data = host_stl;
        dev_stl_data.facets_v1 = d_facets_v1;
        dev_stl_data.facets_v2 = d_facets_v2;
        dev_stl_data.facets_v3 = d_facets_v3;
        dev_stl_data.facets_normal = d_facets_normal;
        dev_stl_data.V21 = d_V21;
        dev_stl_data.V21invV21dotV21 = d_V21invV21dotV21;
        dev_stl_data.V32 = d_V32;
        dev_stl_data.V32invV32dotV32 = d_V32invV32dotV32;
        dev_stl_data.V13 = d_V13;
        dev_stl_data.V13invV13dotV13 = d_V13invV13dotV13;

        err = cudaMemcpy(dev_stl, &dev_stl_data, sizeof(StlParams), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (stl params copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
            cudaFree(dev_nodes);
            cudaFree(dev_stl);
            cudaFree(d_facets_v1); cudaFree(d_facets_v2); cudaFree(d_facets_v3); cudaFree(d_facets_normal);
            cudaFree(d_V21); cudaFree(d_V21invV21dotV21); cudaFree(d_V32); cudaFree(d_V32invV32dotV32);
            cudaFree(d_V13); cudaFree(d_V13invV13dotV13);
            return;
        }

        // 计算网格和块大小
        int threadsPerBlock = 512;
        int totalThreads = numNodes * 8;
        const int maxThreadsPerLaunch = 100'000'000;
        int threadsProcessed = 0;

        while (threadsProcessed < totalThreads) {
            int threadsThisBatch = totalThreads - threadsProcessed;
            if (threadsThisBatch > maxThreadsPerLaunch) {
                threadsThisBatch = maxThreadsPerLaunch;
            }
            int grid = (threadsThisBatch + threadsPerBlock - 1) / threadsPerBlock;

            sum_stl_kernel << <grid, threadsPerBlock >> > (
                reinterpret_cast<CudaNodeData*>((char*)dev_nodes + (threadsProcessed / 8) * sizeof(CudaNodeData)),
                threadsThisBatch / 8,
                dev_stl
                );

            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (kernel execution): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_stl);
                cudaFree(d_facets_v1); cudaFree(d_facets_v2); cudaFree(d_facets_v3); cudaFree(d_facets_normal);
                cudaFree(d_V21); cudaFree(d_V21invV21dotV21); cudaFree(d_V32); cudaFree(d_V32invV32dotV32);
                cudaFree(d_V13); cudaFree(d_V13invV13dotV13);
                return;
            }

            err = cudaDeviceSynchronize();
            if (err != cudaSuccess) {
                fprintf(stderr, "CUDA Error (synchronize): %s\n", cudaGetErrorString(err));
                fflush(stderr);
                cudaFree(dev_nodes);
                cudaFree(dev_stl);
                cudaFree(d_facets_v1); cudaFree(d_facets_v2); cudaFree(d_facets_v3); cudaFree(d_facets_normal);
                cudaFree(d_V21); cudaFree(d_V21invV21dotV21); cudaFree(d_V32); cudaFree(d_V32invV32dotV32);
                cudaFree(d_V13); cudaFree(d_V13invV13dotV13);
                return;
            }
            threadsProcessed += threadsThisBatch;
        }

        // 拷贝结果回主机
        err = cudaMemcpy(host_nodes, dev_nodes, numNodes * sizeof(CudaNodeData), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error (result copy): %s\n", cudaGetErrorString(err));
            fflush(stderr);
        }

        // 释放设备内存
        cudaFree(dev_nodes);
        cudaFree(dev_stl);
        cudaFree(d_facets_v1); cudaFree(d_facets_v2); cudaFree(d_facets_v3); cudaFree(d_facets_normal);
        cudaFree(d_V21); cudaFree(d_V21invV21dotV21); cudaFree(d_V32); cudaFree(d_V32invV32dotV32);
        cudaFree(d_V13); cudaFree(d_V13invV13dotV13);

        fflush(stdout);
    }

}
