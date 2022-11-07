import "gbuftools.i";
import "brdftools.i";
import "deftools.i";
import "skintools.i";
///////////////////////////////////////////////////////////////
// Interfaces
///////////////////////////////////////////////////////////////
uniform_set ub_vtx {
	mat4 m;
	mat4 mv;
	mat4 vp;
	mat4 mvp;
	mat4 mvp_l;
	mat4 mvp_r;
	mat3 mrot;
	vec4 modcolor;
	vec2 InvViewportSize; // inverse target size
	sampler2D InstanceMatrices;
	sampler2D InstanceColors;
	usampler2D InstanceIds;
}
///////////////////////////////////////////////////////////////
uniform_set ub_frg {
  sampler2D ColorMap;
  sampler2D NormalMap;
  sampler2D MtlRufMap;
	vec4 ModColor;
	vec2 InvViewportSize; // inverse target size
	float MetallicFactor;
	float RoughnessFactor;
}

uniform_set ub_frg_fwd {
  sampler2D ColorMap;
  sampler2D NormalMap;
  sampler2D MtlRufMap;

	sampler2D MapBrdfIntegration;
	sampler2D MapSpecularEnv;
	sampler2D MapDiffuseEnv;

	float SkyboxLevel;
  float SpecularLevel;
  float DiffuseLevel;
	vec3 AmbientLevel;

	float MetallicFactor;
	float RoughnessFactor;

  float DepthFogDistance;
  float DepthFogPower;
  vec4 ShadowParams;

  float EnvironmentMipBias;
  float EnvironmentMipScale;

	vec4 ModColor;
	//vec2 InvViewportSize; // inverse target size
	vec3 EyePostion;
}
///////////////////////////////////////////////////////////////
// Vertex Interfaces
///////////////////////////////////////////////////////////////
vertex_interface iface_vgbuffer
	: ub_vtx {
  inputs {
    vec4 position : POSITION;
    vec3 normal : NORMAL;
    vec3 binormal : BINORMAL;
    vec4 vtxcolor : COLOR0;
    vec2 uv0 : TEXCOORD0;
	}
  outputs {
  	vec4 frg_wpos;
    vec4 frg_clr;
    vec2 frg_uv0;
    mat3 frg_tbn;
		float frg_camdist;
		vec3 frg_camz;
	}
}
///////////////////////////////////////////////////////////////
vertex_interface iface_vgbuffer_instanced : iface_vgbuffer {
  outputs {
    vec4 frg_modcolor;
	}
}
///////////////////////////////////////////////////////////////
vertex_interface iface_vgbuffer_stereo : iface_vgbuffer {
  outputs {
    layout(secondary_view_offset=1) int gl_Layer;
  }
}
///////////////////////////////////////////////////////////////
vertex_interface iface_vgbuffer_stereo_instanced : iface_vgbuffer_instanced {
  outputs {
    layout(secondary_view_offset=1) int gl_Layer;
  }
}
///////////////////////////////////////////////////////////////
vertex_interface iface_vgbuffer_skinned
	: iface_vgbuffer
	: iface_skintools {
}
///////////////////////////////////////////////////////////////
// Fragmentertex Interfaces
///////////////////////////////////////////////////////////////
fragment_interface iface_forward
	: ub_frg_fwd {
  inputs {
  	vec4 frg_wpos;
    vec4 frg_clr;
  	vec2 frg_uv0;
  	mat3 frg_tbn;
		float frg_camdist;
		vec3 frg_camz;
	}
	outputs {
		layout(location = 0) vec4 out_color;
	}
}
///////////////////////////////////////////////////////////////
fragment_interface iface_fgbuffer
	: ub_frg {
  inputs {
    vec4 frg_wpos;
    vec4 frg_clr;
  	vec2 frg_uv0;
  	mat3 frg_tbn;
		float frg_camdist;
		vec3 frg_camz;
	}
	outputs {
		layout(location = 0) uvec4 out_gbuf;
	}
}
///////////////////////////////////////////////////////////////
fragment_interface iface_fgbuffer_instanced
	: iface_fgbuffer {
  inputs {
    vec4 frg_modcolor;
	}
}
///////////////////////////////////////////////////////////////
// StateBlocks
///////////////////////////////////////////////////////////////
state_block sb_default : default {
}
///////////////////////////////////////////////////////////////
// Library Blocks
///////////////////////////////////////////////////////////////
libblock lib_pbr_vtx {
	void vs_common(vec4 pos, vec3 nrm, vec3 bin) {
		vec4 cpos  = mv * pos;
    vec3 wnormal = normalize(mrot*normal);
    vec3 wbitangent = normalize(mrot*binormal); // technically binormal is a bitangent
		vec3 wtangent = cross(wbitangent,wnormal);
		//frg_clr = vtxcolor;
		frg_wpos = m*pos;
		frg_clr = vec4(1,1,1,1); //TODO - split vs_rigid_gbuffer into vertexcolor vs identity
		frg_uv0 = uv0*vec2(1,-1);
		frg_tbn = mat3(
	        wtangent,
    	    wbitangent,
					wnormal
        );
		frg_camz = wnormal.xyz;
		frg_camdist = -cpos.z;
	}
}
///////////////////////////////////////////////////////////////
libblock lib_pbr_vtx_instanced {
	void vs_instanced(vec4 pos, vec3 nrm, vec3 bin,mat4 instance_matrix) {
		mat3 instance_rot = mat3(instance_matrix);
		vec4 cpos  = mv * (instance_matrix*pos);
    vec3 wnormal = normalize(instance_rot*normal);
    vec3 wbitangent = normalize(instance_rot*binormal); // technically binormal is a bitangent
		vec3 wtangent = cross(wbitangent,wnormal);
		//frg_clr = vtxcolor;
		frg_wpos = m*(instance_matrix*pos);
		frg_clr = vec4(1,1,1,1); //TODO - split vs_rigid_gbuffer into vertexcolor vs identity
		frg_uv0 = uv0*vec2(1,-1);
		frg_tbn = mat3(wtangent,
    	    wbitangent,
					wnormal
    	);
		frg_camz = wnormal.xyz;
		frg_camdist = -cpos.z;
		////////////////////////////////
		int modcolor_u = (gl_InstanceID&0xfff);
		int modcolor_v = (gl_InstanceID>>12);
		frg_modcolor = texelFetch(InstanceColors, ivec2(modcolor_u,modcolor_v), 0);
		////////////////////////////////
	}
}
///////////////////////////////////////////////////////////////
libblock lib_pbr_frg : lib_gbuf_encode {
	void ps_common_n(vec4 modc, vec3 N,vec2 UV,bool emissive) {
		vec3 normal = normalize(frg_tbn*N);
		vec3 rufmtlamb = texture(MtlRufMap,UV).zyx;
		float mtl = rufmtlamb.x * MetallicFactor;
		float ruf = rufmtlamb.y * RoughnessFactor;
		vec3 color = (modc*frg_clr*texture(ColorMap,UV)).xyz;
		out_gbuf = packGbuffer(color,normal,ruf,mtl,emissive);
	}
	void ps_common_vizn(vec4 modc, vec3 N) {
		vec3 normal = normalize(frg_tbn*N);
		//vec3 rufmtlamb = texture(MtlRufMap,UV).zyx;
		float mtl = 0; //rufmtlamb.x * MetallicFactor;
		float ruf = 1; //rufmtlamb.y * RoughnessFactor;
		//vec3 color = (modc*frg_clr*texture(ColorMap,UV)).xyz;
		out_gbuf = packGbuffer(normal,vec3(0,0,0),ruf,mtl,true);
	}
}
///////////////////////////////////////////////////////////////
// shaders
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// vs-non-instanced-rigid
///////////////////////////////////////////////////////////////
vertex_shader vs_forward_test
	: iface_vgbuffer
  : lib_pbr_vtx {
		vs_common(position,normal,binormal);
		gl_Position = mvp*position;
}
///////////////////////////////////////////////////////////////
// vs-instanced-rigid
///////////////////////////////////////////////////////////////
vertex_shader vs_forward_instanced
	: iface_vgbuffer_instanced
  : lib_pbr_vtx_instanced {
    int matrix_v = (gl_InstanceID>>10);
		int matrix_u = (gl_InstanceID&0x3ff)<<2;
		mat4 instancemtx = mat4(
        texelFetch(InstanceMatrices, ivec2(matrix_u+0,matrix_v), 0),
 		    texelFetch(InstanceMatrices, ivec2(matrix_u+1,matrix_v), 0),
		    texelFetch(InstanceMatrices, ivec2(matrix_u+2,matrix_v), 0),
		    texelFetch(InstanceMatrices, ivec2(matrix_u+3,matrix_v), 0));
  	////////////////////////////////
		vec4 instanced_pos = (instancemtx*position);
		vs_instanced(position,normal,binormal,instancemtx);
		////////////////////////////////
		gl_Position = mvp*instanced_pos;
}
///////////////////////////////////////////////////////////////
// vs-non-instanced-rigid
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer
	: iface_vgbuffer
  : lib_pbr_vtx {
		vs_common(position,normal,binormal);
		gl_Position = mvp*position;
}
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer_stereo
	: extension(GL_NV_stereo_view_rendering)
  : extension(GL_NV_viewport_array2)
	: iface_vgbuffer_stereo
	: lib_pbr_vtx {
	  vs_common(position,normal,binormal);
	  gl_Position = mvp_l*position;
	  gl_SecondaryPositionNV = mvp_r*position;
	  gl_Layer = 0;
	  gl_ViewportMask[0] = 1;
	  gl_SecondaryViewportMaskNV[0] = 2;
}
///////////////////////////////////////////////////////////////
// vs-instanced-rigid
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer_instanced
	: iface_vgbuffer_instanced
  : lib_pbr_vtx_instanced {
    int matrix_v = (gl_InstanceID>>10);
		int matrix_u = (gl_InstanceID&0x3ff)<<2;
		mat4 instancemtx = mat4(
        texelFetch(InstanceMatrices, ivec2(matrix_u+0,matrix_v), 0),
 		    texelFetch(InstanceMatrices, ivec2(matrix_u+1,matrix_v), 0),
		    texelFetch(InstanceMatrices, ivec2(matrix_u+2,matrix_v), 0),
		    texelFetch(InstanceMatrices, ivec2(matrix_u+3,matrix_v), 0));
  	////////////////////////////////
		vec4 instanced_pos = (instancemtx*position);
		vs_instanced(position,normal,binormal,instancemtx);
		////////////////////////////////
		gl_Position = mvp*instanced_pos;
}
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer_instanced_stereo
	: extension(GL_NV_stereo_view_rendering)
  : extension(GL_NV_viewport_array2)
	: iface_vgbuffer_stereo_instanced
	: lib_pbr_vtx_instanced {
		////////////////////////////////
    int matrix_v = (gl_InstanceID>>10);
		int matrix_u = (gl_InstanceID&0x3ff)<<2;
		mat4 instancemtx = mat4(
			texelFetch(InstanceMatrices, ivec2(matrix_u+0,matrix_v), 0),
		  texelFetch(InstanceMatrices, ivec2(matrix_u+1,matrix_v), 0),
		  texelFetch(InstanceMatrices, ivec2(matrix_u+2,matrix_v), 0),
		  texelFetch(InstanceMatrices, ivec2(matrix_u+3,matrix_v), 0));
		////////////////////////////////
		vec4 instanced_pos = (instancemtx*position);
		vs_instanced(position,normal,binormal,instancemtx);
		////////////////////////////////
	  gl_Position = mvp_l*instanced_pos;
	  gl_SecondaryPositionNV = mvp_r*instanced_pos;
		////////////////////////////////
	  gl_Layer = 0;
	  gl_ViewportMask[0] = 1;
	  gl_SecondaryViewportMaskNV[0] = 2;
}
///////////////////////////////////////////////////////////////
// vs-non-instanced-skinned
///////////////////////////////////////////////////////////////
vertex_shader vs_skinned_gbuffer
	: iface_vgbuffer_skinned
  : skin_tools
  : lib_pbr_vtx {
  vec4 skn_pos = vec4(SkinPosition(position.xyz),1);
	vec3 skn_nrm  = SkinNormal(normal);
	vec3 skn_bit  = SkinNormal(binormal); // // technically binormal is a bitangent
	vs_common(skn_pos,skn_nrm,skn_bit);
	gl_Position = mvp*skn_pos;
}
///////////////////////////////////////////////////////////////
// fragment shaders
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer
	: iface_fgbuffer
  : lib_pbr_frg {
	ps_common_n(ModColor,vec3(0,0,1),frg_uv0);
}
///////////////////////////////////////////////////////////////
// vs-non-instanced-rigid
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer_vtxcolor
	: iface_vgbuffer
  : lib_pbr_vtx {
		vs_common(position,normal,binormal);
		frg_clr = vtxcolor;
		gl_Position = mvp*position;
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_vtxcolor
	: iface_fgbuffer
  : lib_pbr_frg {
	out_gbuf = packGbuffer(frg_clr.xyz,vec3(0,0,0),1,0,true);
}
///////////////////////////////////////////////////////////////
vertex_shader vs_rigid_gbuffer_font
	: iface_vgbuffer
  : lib_pbr_vtx {
		vs_common(position,normal,binormal);
		frg_clr = vtxcolor;
		frg_uv0 = uv0;
		gl_Position = mvp*position;
}
vertex_shader vs_rigid_gbuffer_font_instanced
	: iface_vgbuffer
  : lib_pbr_vtx {
		vs_common(position,normal,binormal);
		frg_clr = vtxcolor;
		frg_uv0 = uv0;
		gl_Position = (vp*m)*position;
}
fragment_shader ps_gbuffer_font
	: iface_fgbuffer
  : lib_pbr_frg {
  vec4 s = texture(ColorMap, frg_uv0.xy);
  float texa = pow(s.a*s.r,0.75);
	if(s.r<0.001)
		discard;
	//vec3 color = s.rrr;
	out_gbuf = packGbufferA(1,s.a*s.r);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_vizn
	: iface_fgbuffer
  : lib_pbr_frg {
	ps_common_vizn(ModColor,vec3(0,0,1));
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_n // normalmap
	: iface_fgbuffer
  : lib_pbr_frg {
		vec3 TN = texture(NormalMap,frg_uv0).xyz;
		TN = mix(TN,vec3(0.5,1,0.5),0.0);
		bool emissive = length(TN)<0.1;
		vec3 N = normalize(TN*2.0-vec3(1,1,1));
	ps_common_n(ModColor,N,frg_uv0,emissive);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_n_instanced
	: iface_fgbuffer_instanced
  : lib_pbr_frg {
		vec3 TN = texture(NormalMap,frg_uv0).xyz;
		TN = mix(TN,vec3(0.5,1,0.5),0.0);
		bool emissive = length(TN)<0.1;
		vec3 N = normalize(TN*2.0-vec3(1,1,1));
	ps_common_n(frg_modcolor,N,frg_uv0,emissive);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_n_stereo // normalmap
	: iface_fgbuffer
  : lib_pbr_frg {
		vec3 TN = texture(NormalMap,frg_uv0).xyz;
		bool emissive = length(TN)<0.1;
		vec3 N = normalize(TN*2.0-vec3(1,1,1));
		if( length(TN)<0.1 ) N = vec3(0,0,0);
	ps_common_n(ModColor,N,frg_uv0,emissive);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_n_stereo_instanced
	: iface_fgbuffer_instanced
  : lib_pbr_frg {
		vec3 TN = texture(NormalMap,frg_uv0).xyz;
		bool emissive = length(TN)<0.1;
		vec3 N = normalize(TN*2.0-vec3(1,1,1));
		if( length(TN)<0.1 ) N = vec3(0,0,0);
	ps_common_n(frg_modcolor,N,frg_uv0,emissive);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_gbuffer_n_tex_stereo // normalmap (stereo texture - vsplit)
	: iface_fgbuffer
  : lib_pbr_frg {
		vec2 screen_uv   = gl_FragCoord.xy * InvViewportSize;
		bool is_right = bool(screen_uv.x <= 0.5);
		vec2 map_uv = frg_uv0*vec2(1,0.5);
		if( is_right )
			map_uv += vec2(0,0.5);
		vec3 TN = texture(NormalMap,map_uv).xyz;
  	vec3 N = TN*2.0-vec3(1,1,1);
		bool emissive = length(TN)<0.1;
		ps_common_n(ModColor,N,map_uv,emissive);
}
///////////////////////////////////////////////////////////////
fragment_shader ps_forward_test 
	: iface_forward
	: lib_math
  : lib_brdf
  : lib_def {
		vec3 TN = texture(NormalMap,frg_uv0).xyz;
  	vec3 N = TN*2.0-vec3(1,1,1);
		vec3 normal = normalize(frg_tbn*N);
		vec3 rufmtlamb = texture(MtlRufMap,frg_uv0).zyx;

		PbrData pbd;
		pbd._emissive = length(TN)<0.1;
		pbd._metallic = rufmtlamb.x * MetallicFactor;
		pbd._roughness = rufmtlamb.y * RoughnessFactor;
		pbd._albedo = (ModColor*frg_clr*texture(ColorMap,frg_uv0)).xyz;
		pbd._wpos = frg_wpos.xyz;
		pbd._wnrm = normal;
		pbd._fogZ = 0.0;
		pbd._atmos = 0.0;
		pbd._alpha = 1.0;
		vec3 rgb = pbrEnvironmentLightingXXX(pbd);

		out_color = vec4(rgb,1);
}