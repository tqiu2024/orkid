//

uniform_block ub_vtx_boneblock {
  mat4 _bone_matrices[1024];
}

vertex_interface iface_skintools : ub_vtx_boneblock {
  inputs {
    vec4 boneindices : BONEINDICES;
    vec4 boneweights : BONEWEIGHTS;
  }
}

libblock skin_tools {
  vec3 SkinPosition(vec3 objpos) {
    ivec4 idcsi = ivec4(boneindices);
    vec4 pos4   = vec4(objpos, 1.0);
    vec3 WeightedVertex = ((_bone_matrices[idcsi.w] * pos4).xyz * boneweights.w);
    WeightedVertex += ((_bone_matrices[idcsi.z] * pos4).xyz * boneweights.z);
    WeightedVertex += ((_bone_matrices[idcsi.y] * pos4).xyz * boneweights.y);
    WeightedVertex += ((_bone_matrices[idcsi.x] * pos4).xyz * boneweights.x);
    return WeightedVertex;
  }
  vec3 SkinNormal(vec3 InNrm) {
    ivec4 idcss = ivec4(boneindices);
    vec4 Nrm4   = vec4(InNrm, 0.0);

    vec3 WeightedNormal = ((_bone_matrices[idcss.w] * Nrm4) * boneweights.w).xyz;
    WeightedNormal += ((_bone_matrices[idcss.z] * Nrm4) * boneweights.z).xyz;
    WeightedNormal += ((_bone_matrices[idcss.y] * Nrm4) * boneweights.y).xyz;
    WeightedNormal += ((_bone_matrices[idcss.x] * Nrm4) * boneweights.x).xyz;

    return normalize(WeightedNormal);
  }

  struct SkinOut {
    vec3 skn_pos;
    vec3 skn_col;
  };

  SkinOut LitSkinned(vec3 objpos) {
		SkinOut rval;
		rval.skn_pos = SkinPosition(position.xyz);
	  vec3 sknorm  = SkinNormal(normal.xyz);
	  vec3 wnorm   = normalize(mrot * sknorm);
	  float dif = dot(wnorm, vec3(0, 0, 1));
	  float amb = 0.3;
	  float tot = dif + amb;
		rval.skn_col = vec3(tot,tot,tot);
		return rval;
	}
}
