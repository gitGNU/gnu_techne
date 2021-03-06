layout(vertices = 1) out;

in vec3 apex_v[], left_v[], right_v[], normal_v[], color_v[];
in float score_v[];
in float clustering_v[];
in unsigned int instance_v[];

patch out vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch out float score_tc, depth_tc;
patch out unsigned int instance_tc;

uniform grass_control {
    float detail, rolloff;
};

vec3 cluster_stratum(vec3 apex, vec3 left, vec3 right, unsigned int instance);

void main()
{
    vec4 p_e;
    vec3 p;
    float z, n;
    int i;

    /* Calculate the distance from the seeded triangle center to the
     * viewer to determine the detail of the plants in the cluster. */

    p = (apex_v[0] + left_v[0] + right_v[0]) / 3;
    p_e = modelview * vec4(p, 1);
    z = max(-p_e.z, 1);
    n = detail / z / z;

    gl_TessLevelOuter[0] = clustering_v[0];
    gl_TessLevelOuter[1] = n;

    apex_tc = apex_v[0];
    left_tc = left_v[0];
    right_tc = right_v[0];
    color_tc = color_v[0];
    normal_tc = normal_v[0];
    score_tc = score_v[0];
    depth_tc = z;
    instance_tc = instance_v[0];
    stratum_tc = cluster_stratum(apex_v[0], left_v[0], right_v[0],
                                 instance_v[0]);
}
