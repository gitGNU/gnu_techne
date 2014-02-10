<@begin main @>
layout(points) in;
layout(points, max_vertices = 1) out;
              
in vec4 color_v[];
in float distance_v[];
in int index_v[];
in vec3 apex_v[], left_v[], right_v[], stratum_v[];
in uvec2 chance_v[];

<@ for i = 0, context.n - 1 do @>
layout(stream = <@= i @>) out stream_<@= i @> {
    vec4 stream_<@= i @>_color_g;
    vec3 stream_<@= i @>_apex_g, stream_<@= i @>_left_g, stream_<@= i @>_right_g, stream_<@= i @>_stratum_g;
    float stream_<@= i @>_distance_g;              
    uvec2 stream_<@= i @>_chance_g;
    float stream_<@= i @>_clustering_g;
};
<@ end @>

uniform float clustering;

uniform vegetation_debug{
    int debug;
    bool debugtoggle;
};

void main()
{
    switch (index_v[0]) {
    <@ for i = 0, context.n - 1 do @>
    case <@= i @>:
        stream_<@= i @>_color_g = color_v[0];    
        stream_<@= i @>_distance_g = distance_v[0];
        stream_<@= i @>_apex_g = apex_v[0];
        stream_<@= i @>_left_g = left_v[0];
        stream_<@= i @>_right_g = right_v[0];
        stream_<@= i @>_stratum_g = stratum_v[0];
        stream_<@= i @>_clustering_g = clustering;
        stream_<@= i @>_chance_g = chance_v[0];

        EmitStreamVertex(<@= i @>);
        EndStreamPrimitive(<@= i @>);

        break;
    <@ end @>
    }
}
<@finish @>
