
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include "../d3dx/d3dx9.h"

#include "ss3d_engine.h"


#define D3DCALL( x, m ) (x)->lpVtbl->m( (x) )
#define D3DCALL_( x, m, ... ) (x)->lpVtbl->m( (x), __VA_ARGS__ )
#define SAFE_RELEASE( x ) if( x ){ D3DCALL( x, Release ); x = NULL; }


typedef struct _SS3D_Shader_D3D9
{
	IDirect3DPixelShader9* PS;
	IDirect3DVertexShader9* VS;
	ID3DXConstantTable* PSCT;
	ID3DXConstantTable* VSCT;
}
SS3D_Shader_D3D9;

typedef struct _SS3D_Texture_D3D9
{
	SS3D_Texture inh;
	
	union
	{
		IDirect3DBaseTexture9* base;
		IDirect3DTexture9* tex2d;
		IDirect3DCubeTexture9* cube;
		IDirect3DVolumeTexture9* vol;
	}
	ptr;
}
SS3D_Texture_D3D9;

typedef struct _SS3D_RenderTexture_D3D9
{
	SS3D_Texture_D3D9 inh; /* contains size info and color (output 0) texture */
	
	IDirect3DSurface9* CS; /* color (output 0) surface */
	IDirect3DTexture9* DT; /* depth (output 2) texture (not used for shadowmaps, such data in CT/CS already) */
	IDirect3DSurface9* DS; /* depth (output 2) surface (not used for shadowmaps, such data in CT/CS already) */
	IDirect3DSurface9* DSS; /* depth/stencil surface */
}
SS3D_RenderTexture_D3D9;

typedef struct _SS3D_VDecl_D3D9
{
	SS3D_VDeclInfo info;
	
	struct _SS3D_RD3D9* renderer;
	IDirect3DVertexDeclaration9* VD;
}
SS3D_VDecl_D3D9;

typedef struct _SS3D_Mesh_D3D9
{
	SS3D_Mesh inh;
	
	IDirect3DVertexBuffer9* VB;
	IDirect3DIndexBuffer9* IB;
}
SS3D_Mesh_D3D9;

typedef struct RTData
{
	IDirect3DTexture9* RTT_OCOL; /* oR, oG, oB, oA, RGBA16F */
	IDirect3DTexture9* RTT_PARM; /* distX, distY, emissive, oA (blending purposes), RGBA16F */
	IDirect3DTexture9* RTT_DEPTH; /* depth, R32F */
	IDirect3DTexture9* RTT_BLOOM_DSHP; /* bloom downsample/high-pass RT, RGBA8 */
	IDirect3DTexture9* RTT_BLOOM_BLUR1; /* bloom horizontal blur RT, RGBA8 */
	IDirect3DTexture9* RTT_BLOOM_BLUR2; /* bloom vertical blur RT, RGBA8 */
	
	IDirect3DSurface9* RTS_OCOL;
	IDirect3DSurface9* RTS_PARM;
	IDirect3DSurface9* RTS_DEPTH;
	IDirect3DSurface9* RTS_BLOOM_DSHP;
	IDirect3DSurface9* RTS_BLOOM_BLUR1;
	IDirect3DSurface9* RTS_BLOOM_BLUR2;
	IDirect3DSurface9* RTSD;
	int width, height;
}
RTData;

typedef struct _SS3D_RD3D9
{
	SS3D_Renderer inh;
	
	IDirect3DDevice9* device;
	IDirect3DSurface9* bb_color;
	IDirect3DSurface9* bb_depth;
	RTData drd;
}
SS3D_RD3D9;


//
//  P O S T  -  P R O C E S S
//

static void postproc_init( SS3D_RD3D9* R, RTData* D, int w, int h )
{
	HRESULT hr;
	memset( D, 0, sizeof(*D) );
	
	D->width = w;
	D->height = h;
	
	int w4 = w / 4;
	int h4 = h / 4;
	
	/* core */
	hr = D3DCALL_( R->device, CreateTexture, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &D->RTT_OCOL, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_OCOL || !"failed to create rgba16f render target texture" );
	
	hr = D3DCALL_( R->device, CreateTexture, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &D->RTT_PARM, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_PARM || !"failed to create rgba16f render target texture" );
	
	hr = D3DCALL_( R->device, CreateTexture, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &D->RTT_DEPTH, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_PARM || !"failed to create r32f depth stencil texture" );
	
	/* bloom */
	hr = D3DCALL_( R->device, CreateTexture, w4, h4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &D->RTT_BLOOM_DSHP, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_BLOOM_DSHP || !"failed to create rgba16f render target texture" );
	
	hr = D3DCALL_( R->device, CreateTexture, w4, h4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &D->RTT_BLOOM_BLUR1, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_BLOOM_BLUR1 || !"failed to create rgba16f render target texture" );
	
	hr = D3DCALL_( R->device, CreateTexture, w4, h4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &D->RTT_BLOOM_BLUR2, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTT_BLOOM_BLUR2 || !"failed to create rgba16f render target texture" );
	
	/* depth */
	hr = D3DCALL_( R->device, CreateDepthStencilSurface, w, h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &D->RTSD, NULL );
	sgs_BreakIf( FAILED( hr ) || !D->RTSD || !"failed to create d24s8 depth+stencil surface" );
	
	/* surfaces */
	D3DCALL_( D->RTT_OCOL, GetSurfaceLevel, 0, &D->RTS_OCOL );
	D3DCALL_( D->RTT_PARM, GetSurfaceLevel, 0, &D->RTS_PARM );
	D3DCALL_( D->RTT_DEPTH, GetSurfaceLevel, 0, &D->RTS_DEPTH );
	D3DCALL_( D->RTT_BLOOM_DSHP, GetSurfaceLevel, 0, &D->RTS_BLOOM_DSHP );
	D3DCALL_( D->RTT_BLOOM_BLUR1, GetSurfaceLevel, 0, &D->RTS_BLOOM_BLUR1 );
	D3DCALL_( D->RTT_BLOOM_BLUR2, GetSurfaceLevel, 0, &D->RTS_BLOOM_BLUR2 );
}

static void postproc_free( SS3D_RD3D9* R, RTData* D )
{
	SAFE_RELEASE( D->RTS_OCOL );
	SAFE_RELEASE( D->RTS_PARM );
	SAFE_RELEASE( D->RTS_DEPTH );
	SAFE_RELEASE( D->RTS_BLOOM_DSHP );
	SAFE_RELEASE( D->RTS_BLOOM_BLUR1 );
	SAFE_RELEASE( D->RTS_BLOOM_BLUR2 );
	SAFE_RELEASE( D->RTT_OCOL );
	SAFE_RELEASE( D->RTT_PARM );
	SAFE_RELEASE( D->RTT_DEPTH );
	SAFE_RELEASE( D->RTT_BLOOM_DSHP );
	SAFE_RELEASE( D->RTT_BLOOM_BLUR1 );
	SAFE_RELEASE( D->RTT_BLOOM_BLUR2 );
	SAFE_RELEASE( D->RTSD );
	D->width = 0;
	D->height = 0;
}


//
//  S H A D E R S
//

static void use_shader( SS3D_RD3D9* R, SS3D_Shader_D3D9* S )
{
	D3DCALL_( R->device, SetPixelShader, S ? S->PS : NULL );
	D3DCALL_( R->device, SetVertexShader, S ? S->VS : NULL );
}

static void vshc_set_vec4array( SS3D_RD3D9* R, int pos, VEC4 data, int count ){ D3DCALL_( R->device, SetVertexShaderConstantF, pos, data, count ); }
static void vshc_set_mat4( SS3D_RD3D9* R, int pos, MAT4 mtx ){ D3DCALL_( R->device, SetVertexShaderConstantF, pos, mtx[0], 4 ); }

static void pshc_set_float( SS3D_RD3D9* R, int pos, float f ){ VEC4 v = { f, f, f, f }; D3DCALL_( R->device, SetPixelShaderConstantF, pos, v, 1 ); }
static void pshc_set_vec4array( SS3D_RD3D9* R, int pos, VEC4 data, int count ){ D3DCALL_( R->device, SetPixelShaderConstantF, pos, data, count ); }
// static void pshc_set_int( SS3D_RD3D9* R, int pos, int i ){ VEC4 v = { i, i, i, i }; D3DCALL_( R->device, SetPixelShaderConstantF, pos, v, 1 ); }
static void pshc_set_mat4( SS3D_RD3D9* R, int pos, MAT4 mtx ){ D3DCALL_( R->device, SetPixelShaderConstantF, pos, mtx[0], 4 ); }

static void shd3d9_free( SS3D_Shader_D3D9* S )
{
	SAFE_RELEASE( S->PSCT );
	SAFE_RELEASE( S->VSCT );
	SAFE_RELEASE( S->PS );
	SAFE_RELEASE( S->VS );
}

static int shd3d9_init_source( SS3D_RD3D9* R, SS3D_Shader_D3D9* S, const char* code, int codelen )
{
	SGS_CTX = R->inh.C;
	HRESULT hr;
	ID3DXBuffer *outbuf = NULL, *outerr = NULL;
	
	memset( S, 0, sizeof(*S) );
	
	static const D3DXMACRO vsmacros[] = { { "VS", "1" }, { NULL, NULL } };
	static const D3DXMACRO psmacros[] = { { "PS", "1" }, { NULL, NULL } };
	
	hr = D3DXCompileShader( code, codelen, vsmacros, NULL, "main", "vs_3_0", D3DXSHADER_OPTIMIZATION_LEVEL3, &outbuf, &outerr, &S->VSCT );
	if( FAILED( hr ) )
	{
		if( outerr )
			sgs_Msg( C, SGS_WARNING, "Errors in vertex shader compilation:\n%s", (const char*) D3DCALL( outerr, GetBufferPointer ) );
		else
			sgs_Msg( C, SGS_WARNING, "Unknown error in vertex shader compilation" );
		if( outbuf ) SAFE_RELEASE( outbuf );
		if( outerr ) SAFE_RELEASE( outerr );
		goto cleanup;
	}
	hr = IDirect3DDevice9_CreateVertexShader( R->device, (const DWORD*) D3DCALL( outbuf, GetBufferPointer ), &S->VS );
	SAFE_RELEASE( outbuf );
	if( outerr ) SAFE_RELEASE( outerr );
	if( FAILED( hr ) )
	{
		sgs_Msg( C, SGS_WARNING, "Unknown error while loading vertex shader" );
		goto cleanup;
	}
	
	hr = D3DXCompileShader( code, codelen, psmacros, NULL, "main", "ps_3_0", D3DXSHADER_OPTIMIZATION_LEVEL3, &outbuf, &outerr, &S->PSCT );
	if( FAILED( hr ) )
	{
		if( outerr )
			sgs_Msg( C, SGS_WARNING, "Errors in pixel shader compilation:\n%s", (const char*) D3DCALL( outerr, GetBufferPointer ) );
		else
			sgs_Msg( C, SGS_WARNING, "Unknown error in pixel shader compilation" );
		if( outbuf ) SAFE_RELEASE( outbuf );
		if( outerr ) SAFE_RELEASE( outerr );
		goto cleanup;
	}
	hr = IDirect3DDevice9_CreatePixelShader( R->device, (const DWORD*) D3DCALL( outbuf, GetBufferPointer ), &S->PS );
	SAFE_RELEASE( outbuf );
	if( outerr ) SAFE_RELEASE( outerr );
	if( FAILED( hr ) )
	{
		sgs_Msg( C, SGS_WARNING, "Unknown error while loading pixel shader" );
		goto cleanup;
	}
	
	return 1;
	
cleanup:
	shd3d9_free( S );
	return 0;
}

#define SH_HDR SS3D_Shader_D3D9* S = (SS3D_Shader_D3D9*) data->data;

static int shd3d9_destruct( SGS_CTX, sgs_VarObj* data )
{
	SH_HDR;
	shd3d9_free( S );
	return SGS_SUCCESS;
}

static sgs_ObjInterface SS3D_Shader_D3D9_iface[1] =
{{
	"SS3D_Shader_D3D9",
	shd3d9_destruct, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};

static int get_shader_( SS3D_RD3D9* R, sgs_Variable* key )
{
	SGS_CTX = R->inh.C;
	sgs_VHTVar* found = sgs_vht_get( &R->inh.shaders, key );
	if( found )
	{
		sgs_PushVariable( C, &found->val );
		return 1;
	}
	
	// load shader
	{
		char* buf;
		sgs_SizeVal size;
		sgs_Variable val;
		SS3D_Shader_D3D9 shader, *S;
		
		// load code
		sgs_PushString( C, "d3d9" );
		sgs_PushVariable( C, key );
		sgs_GlobalCall( C, "_SS3D_Shader_LoadCode", 2, 1 );
		if( !sgs_ParseString( C, -1, &buf, &size ) )
			return sgs_Msg( C, SGS_WARNING, "failed to load shader code (%s)", sgs_var_cstr( key ) );
		
		// compile code
		if( !shd3d9_init_source( R, &shader, buf, size ) )
			return 0;
		
		S = (SS3D_Shader_D3D9*) sgs_PushObjectIPA( C, sizeof(*S), SS3D_Shader_D3D9_iface );
		memcpy( S, &shader, sizeof(*S) );
		sgs_PeekStackItem( C, -1, &val );
		
		sgs_vht_set( &R->inh.shaders, C, key, &val );
		return 1;
	}
}

static SS3D_Shader_D3D9* get_shader( SS3D_RD3D9* R, const char* name )
{
	sgs_Variable key;
	SGS_CTX = R->inh.C;
	sgs_SizeVal ssz = sgs_StackSize( C );
	SS3D_Shader_D3D9* out = NULL;
	
	sgs_PushString( C, name );
	sgs_PeekStackItem( C, -1, &key );
	
	if( get_shader_( R, &key ) )
		out = (SS3D_Shader_D3D9*) sgs_GetObjectData( C, -1 );
	sgs_SetStackSize( C, ssz );
	return out;
}

static sgs_VarObj* get_shader_obj( SS3D_RD3D9* R, const char* name )
{
	sgs_Variable key;
	SGS_CTX = R->inh.C;
	sgs_SizeVal ssz = sgs_StackSize( C );
	sgs_VarObj* out = NULL;
	
	sgs_PushString( C, name );
	sgs_PeekStackItem( C, -1, &key );
	
	if( get_shader_( R, &key ) )
		out = (sgs_VarObj*) sgs_GetObjectStruct( C, -1 );
	sgs_SetStackSize( C, ssz );
	return out;
}


//
//  T E X T U R E S
//

static void use_texture( SS3D_RD3D9* R, int slot, SS3D_Texture_D3D9* T )
{
	D3DCALL_( R->device, SetTexture, slot, T ? T->ptr.base : NULL );
	if( T )
	{
		D3DCALL_( R->device, SetSamplerState, slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		D3DCALL_( R->device, SetSamplerState, slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		D3DCALL_( R->device, SetSamplerState, slot, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	}
}

static D3DFORMAT texfmt2d3d( int fmt )
{
	switch( fmt )
	{
	case SS3DFORMAT_BGRA8:
	case SS3DFORMAT_RGBA8: return D3DFMT_A8R8G8B8;
	case SS3DFORMAT_R5G6B5: return D3DFMT_R5G6B5;
	
	case SS3DFORMAT_DXT1: return D3DFMT_DXT1;
	case SS3DFORMAT_DXT3: return D3DFMT_DXT3;
	case SS3DFORMAT_DXT5: return D3DFMT_DXT5;
	}
	return 0;
}

static void texd3d9_free( SS3D_Texture_D3D9* T )
{
	SAFE_RELEASE( T->ptr.base );
}

static void swap4b2ms( uint32_t* data, int size, int mask1, int shift1R, int mask2, int shift2L )
{
	int i;
	for( i = 0; i < size; ++i )
	{
		uint32_t O = data[i];
		uint32_t N = ( O & ~( mask1 | mask2 ) ) | ( ( O & mask1 ) >> shift1R ) | ( ( O & mask2 ) << shift2L );
		data[i] = N;
	}
}

static void texdatacopy( D3DLOCKED_RECT* plr, SS3D_TextureData* TD, int side, int mip )
{
	int ret;
	uint8_t *src, *dst;
	size_t i, off, copyrowsize = 0, copyrowcount = 0;
	SS3D_TextureInfo mipTI;
	
	off = SS3D_TextureData_GetMipDataOffset( TD, side, mip );
	ret = SS3D_TextureInfo_GetMipInfo( &TD->info, mip, &mipTI );
	sgs_BreakIf( !ret );
	
//	printf( "read side=%d mip=%d at %d\n", side, mip, off );
	
	src = ((uint8_t*)TD->data) + off;
	dst = (uint8_t*)plr->pBits;
	SS3D_TextureInfo_GetCopyDims( &mipTI, &copyrowsize, &copyrowcount );
	
	for( i = 0; i < copyrowcount; ++i )
	{
		memcpy( dst, src, copyrowsize );
		if( TD->info.format == SS3DFORMAT_RGBA8 )
			swap4b2ms( (uint32_t*) dst, copyrowsize / 4, 0xff0000, 16, 0xff, 16 );
		src += copyrowsize;
		dst += plr->Pitch;
	}
}

static int texd3d9_init( SS3D_RD3D9* R, SS3D_Texture_D3D9* T, SS3D_TextureData* TD )
{
	int mip, side;
	HRESULT hr;
	// TODO: filter unsupported formats / dimensions
	
	if( TD->info.type == SS3DTEXTURE_2D )
	{
		IDirect3DTexture9* d3dtex;
		
		hr = D3DCALL_( R->device, CreateTexture, TD->info.width, TD->info.height, TD->info.mipcount, 0, texfmt2d3d( TD->info.format ), D3DPOOL_MANAGED, &d3dtex, NULL );
		if( FAILED( hr ) )
			return sgs_Msg( R->inh.C, SGS_WARNING, "failed to load texture - error while creating d3d9 texture"
				" (type: 2D, w: %d, h: %d, mips: %d, fmt: %d, d3dfmt: %d)", TD->info.width, TD->info.height, TD->info.mipcount, TD->info.format, texfmt2d3d( TD->info.format ) );
		
		// load all mip levels into it
		for( mip = 0; mip < TD->info.mipcount; ++mip )
		{
			D3DLOCKED_RECT lr;
			hr = D3DCALL_( d3dtex, LockRect, mip, &lr, NULL, D3DLOCK_DISCARD );
			if( FAILED( hr ) )
				return sgs_Msg( R->inh.C, SGS_WARNING, "failed to load texture - error while locking d3d9 texture" );
			
			texdatacopy( &lr, TD, 0, mip );
			
			hr = D3DCALL_( d3dtex, UnlockRect, mip );
			if( FAILED( hr ) )
				return sgs_Msg( R->inh.C, SGS_WARNING, "failed to load texture - error while unlocking d3d9 texture" );
		}
		
		T->inh.renderer = &R->inh;
		T->inh.info = TD->info;
		T->ptr.tex2d = d3dtex;
		return 1;
	}
	else if( TD->info.type == SS3DTEXTURE_CUBE )
	{
		IDirect3DCubeTexture9* d3dtex;
		
		hr = D3DCALL_( R->device, CreateCubeTexture, TD->info.width, TD->info.mipcount, 0, texfmt2d3d( TD->info.format ), D3DPOOL_MANAGED, &d3dtex, NULL );
		
		// load all mip levels into it
		for( side = 0; side < 6; ++side )
		{
			for( mip = 0; mip < TD->info.mipcount; ++mip )
			{
				D3DLOCKED_RECT lr;
				hr = D3DCALL_( d3dtex, LockRect, side, mip, &lr, NULL, D3DLOCK_DISCARD );
				if( FAILED( hr ) )
					return sgs_Msg( R->inh.C, SGS_WARNING, "failed to load texture - error while locking d3d9 texture" );
				
				texdatacopy( &lr, TD, side, mip );
				
				hr = D3DCALL_( d3dtex, UnlockRect, side, mip );
				if( FAILED( hr ) )
					return sgs_Msg( R->inh.C, SGS_WARNING, "failed to load texture - error while unlocking d3d9 texture" );
			}
		}
		
		T->inh.renderer = &R->inh;
		T->inh.info = TD->info;
		T->ptr.cube = d3dtex;
		return 1;
	}
	
	return sgs_Msg( R->inh.C, SGS_ERROR, "TODO [reached a part of not-yet-defined behavior]" );
}

#define TEX_HDR SS3D_Texture_D3D9* T = (SS3D_Texture_D3D9*) data->data;

static int texd3d9_destruct( SGS_CTX, sgs_VarObj* data )
{
	TEX_HDR;
	if( T->inh.renderer )
	{
		SS3D_Renderer_PokeResource( T->inh.renderer, data, 0 );
		T->inh.renderer = NULL;
		texd3d9_free( T );
	}
	return SGS_SUCCESS;
}

static sgs_ObjInterface SS3D_Texture_D3D9_iface[1] =
{{
	"SS3D_Texture_D3D9",
	texd3d9_destruct, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};

static int get_texture_( SS3D_RD3D9* R, sgs_Variable* key )
{
	int err;
	SGS_CTX = R->inh.C;
	sgs_VHTVar* found;
	
	sgs_BreakIf( key->type != SGS_VT_STRING );
	
//	printf( "loading texture \"%s\"\n", sgs_var_cstr( key ) );
	
	found = sgs_vht_get( &R->inh.textures, key );
	if( found )
	{
		sgs_PushVariable( C, &found->val );
		return 1;
	}
	
	// load texture
	{
		sgs_Variable val;
		SS3D_TextureData texdata;
		SS3D_Texture_D3D9 texture, *T;
		
		err = SS3D_TextureData_LoadFromFile( &texdata, sgs_var_cstr( key ) );
		if( err != SGS_SUCCESS )
			return sgs_Msg( C, SGS_WARNING, "failed to load texture %s from file", sgs_var_cstr( key ) );
		
		err = !texd3d9_init( R, &texture, &texdata );
		SS3D_TextureData_Free( &texdata );
		if( err )
			return 0;
		
		T = (SS3D_Texture_D3D9*) sgs_PushObjectIPA( C, sizeof(*T), SS3D_Texture_D3D9_iface );
		memcpy( T, &texture, sizeof(*T) );
		sgs_PeekStackItem( C, -1, &val );
		SS3D_Renderer_PokeResource( &R->inh, val.data.O, 1 );
		
		sgs_vht_set( &R->inh.textures, C, key, &val );
		return 1;
	}
}

static SS3D_Texture_D3D9* get_texture( SS3D_RD3D9* R, const char* name )
{
	sgs_Variable key;
	SGS_CTX = R->inh.C;
	sgs_SizeVal ssz = sgs_StackSize( C );
	SS3D_Texture_D3D9* out = NULL;
	
	sgs_PushString( C, name );
	sgs_PeekStackItem( C, -1, &key );
	
	if( get_texture_( R, &key ) )
		out = (SS3D_Texture_D3D9*) sgs_GetObjectData( C, -1 );
	sgs_SetStackSize( C, ssz );
	return out;
}


//
// R T
//

SGS_DECLARE sgs_ObjInterface SS3D_RenderTexture_D3D9_iface[1];

#define RT_HDR SS3D_RenderTexture_D3D9* RT = (SS3D_RenderTexture_D3D9*) obj->data;

static void rtd3d9_free( SS3D_RenderTexture_D3D9* RT )
{
	SAFE_RELEASE( RT->inh.ptr.tex2d );
	SAFE_RELEASE( RT->CS );
	SAFE_RELEASE( RT->DS );
	SAFE_RELEASE( RT->DT );
}

static int rtd3d9_destruct( SGS_CTX, sgs_VarObj* obj )
{
	RT_HDR;
	if( RT->inh.inh.renderer )
	{
		SS3D_Renderer_PokeResource( RT->inh.inh.renderer, obj, 0 );
		RT->inh.inh.renderer = NULL;
		rtd3d9_free( RT );
	}
	return SGS_SUCCESS;
}

static sgs_ObjInterface SS3D_RenderTexture_D3D9_iface[1] =
{{
	"SS3D_RenderTexture_D3D9",
	rtd3d9_destruct, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};

static int rtd3d9_create( SS3D_RD3D9* R, int width, int height, int format )
{
	HRESULT hr = 0;
	D3DFORMAT d3dfmt = 0;
	
	if( width < 1 || width > 4096 || height < 1 || height > 4096 )
		return sgs_Msg( R->inh.C, SGS_WARNING, "illegal size: %d x %d", width, height );
	
	SS3D_RenderTexture_D3D9* RT = (SS3D_RenderTexture_D3D9*)
		sgs_PushObjectIPA( R->inh.C, sizeof(SS3D_RenderTexture_D3D9), SS3D_RenderTexture_D3D9_iface );
	memset( RT, 0, sizeof(*RT) );
	switch( format )
	{
	case SS3DRT_FORMAT_BACKBUFFER:
		d3dfmt = D3DFMT_A16B16G16R16F;
		break;
	case SS3DRT_FORMAT_DEPTH:
		d3dfmt = D3DFMT_R32F;
		break;
	default:
		return sgs_Msg( R->inh.C, SGS_WARNING, "format ID was not recognized / supported: %d", format );
	}
	
	if( format == SS3DRT_FORMAT_DEPTH )
	{
		hr = D3DCALL_( R->device, CreateTexture, width, height, 1, D3DUSAGE_RENDERTARGET, d3dfmt, D3DPOOL_DEFAULT, &RT->inh.ptr.tex2d, NULL );
		sgs_BreakIf( FAILED( hr ) || !RT->inh.ptr.tex2d || !"failed to create render target texture" );
		
		hr = D3DCALL_( R->device, CreateDepthStencilSurface, width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &RT->DSS, NULL );
		sgs_BreakIf( FAILED( hr ) || !RT->DSS || !"failed to create d24s8 depth+stencil surface" );
		
		D3DCALL_( RT->inh.ptr.tex2d, GetSurfaceLevel, 0, &RT->CS );
		
	//	RT->DT = RT->inh.ptr.tex2d;
	//	D3DCALL( RT->DT, AddRef );
	}
	else
	{
		hr = D3DCALL_( R->device, CreateTexture, width, height, 1, D3DUSAGE_RENDERTARGET, d3dfmt, D3DPOOL_DEFAULT, &RT->inh.ptr.tex2d, NULL );
		sgs_BreakIf( FAILED( hr ) || !RT->inh.ptr.tex2d || !"failed to create render target texture" );
		
		hr = D3DCALL_( R->device, CreateTexture, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &RT->DT, NULL );
		sgs_BreakIf( FAILED( hr ) || !RT->DT || !"failed to create r32f depth stencil texture" );
		
		hr = D3DCALL_( R->device, CreateDepthStencilSurface, width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &RT->DSS, NULL );
		sgs_BreakIf( FAILED( hr ) || !RT->DSS || !"failed to create d24s8 depth+stencil surface" );
		
		D3DCALL_( RT->inh.ptr.tex2d, GetSurfaceLevel, 0, &RT->CS );
		D3DCALL_( RT->DT, GetSurfaceLevel, 0, &RT->DS );
	}
	
	RT->inh.inh.renderer = &R->inh;
	RT->inh.inh.info.width = width;
	RT->inh.inh.info.height = height;
	
	SS3D_Renderer_PokeResource( &R->inh, sgs_GetObjectStruct( R->inh.C, -1 ), 1 );
	
	return 1;
}


//
//  V D E C L
//

SGS_DECLARE sgs_ObjInterface SS3D_VDecl_D3D9_iface[1];

#define VD_HDR SS3D_VDecl_D3D9* VD = (SS3D_VDecl_D3D9*) obj->data;

static void vdecld3d9_free( SS3D_VDecl_D3D9* VD )
{
	SAFE_RELEASE( VD->VD );
}

static int vdecld3d9_destruct( SGS_CTX, sgs_VarObj* obj )
{
	VD_HDR;
	if( VD->renderer )
	{
		SS3D_Renderer_PokeResource( &VD->renderer->inh, obj, 0 );
		VD->renderer = NULL;
		vdecld3d9_free( VD );
	}
	return SGS_SUCCESS;
}

static sgs_ObjInterface SS3D_VDecl_D3D9_iface[1] =
{{
	"SS3D_VDecl_D3D9",
	vdecld3d9_destruct, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};

static int vdecltype_to_eltype[] =
{
	D3DDECLTYPE_FLOAT1,
	D3DDECLTYPE_FLOAT2,
	D3DDECLTYPE_FLOAT3,
	D3DDECLTYPE_FLOAT4,
	D3DDECLTYPE_D3DCOLOR,
};

static int vdeclusage_to_elusage[] =
{
	D3DDECLUSAGE_POSITION,
	D3DDECLUSAGE_COLOR,
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_TANGENT,
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_TEXCOORD,
};

static int vdeclusage_to_elusageindex[] = { 0, 0, 0, 0, 0, 1, 2, 3 };

static int init_vdecl( SS3D_RD3D9* R, SS3D_VDecl_D3D9* VD )
{
	int i;
	D3DVERTEXELEMENT9 elements[ SS3D_VDECL_MAX_ITEMS + 1 ], end[1] = { D3DDECL_END() };
	for( i = 0; i < VD->info.count; ++i )
	{
		elements[ i ].Stream = 0;
		elements[ i ].Offset = VD->info.offsets[ i ];
		elements[ i ].Type = vdecltype_to_eltype[ VD->info.types[ i ] ];
		elements[ i ].Method = D3DDECLMETHOD_DEFAULT;
		elements[ i ].Usage = vdeclusage_to_elusage[ VD->info.usages[ i ] ];
		elements[ i ].UsageIndex = vdeclusage_to_elusageindex[ VD->info.usages[ i ] ];
	}
	memcpy( elements + VD->info.count, end, sizeof(*end) );
	if( FAILED( D3DCALL_( R->device, CreateVertexDeclaration, elements, &VD->VD ) ) )
		return sgs_Msg( R->inh.C, SGS_WARNING, "failed to create vertex declaration" );
	return 1;
}

static int create_vdecl( SS3D_RD3D9* R, const char* text )
{
	const char* err;
	SS3D_VDecl_D3D9* VD = (SS3D_VDecl_D3D9*) sgs_PushObjectIPA( R->inh.C, sizeof(*VD), SS3D_VDecl_D3D9_iface );
	memset( VD, 0, sizeof(*VD) );
	err = SS3D_VDeclInfo_Parse( &VD->info, text );
	if( err )
		return sgs_Msg( R->inh.C, SGS_WARNING, "failed to parse vertex declaration: %s", err );
	VD->renderer = R;
	SS3D_Renderer_PokeResource( &R->inh, sgs_GetObjectStruct( R->inh.C, -1 ), 1 );
	return init_vdecl( R, VD );
}


//
//  M E S H
//

SGS_DECLARE sgs_ObjInterface SS3D_Mesh_D3D9_iface[1];

#define M_HDR SS3D_Mesh_D3D9* M = (SS3D_Mesh_D3D9*) obj->data;
#define M_IHDR( funcname ) SS3D_Mesh_D3D9* M; \
	if( !SGS_PARSE_METHOD( C, SS3D_Mesh_D3D9_iface, M, SS3D_Mesh_D3D9, funcname ) ) return 0;

static void meshd3d9_free( SS3D_Mesh_D3D9* M )
{
	SAFE_RELEASE( M->VB );
	SAFE_RELEASE( M->IB );
}

static int meshd3d9_destruct( SGS_CTX, sgs_VarObj* obj )
{
	M_HDR;
	if( M->inh.renderer )
	{
		SS3D_Renderer_PokeResource( M->inh.renderer, obj, 0 );
		M->inh.renderer = NULL;
		meshd3d9_free( M );
	}
	return SGS_SUCCESS;
}

static int meshd3d9_gcmark( SGS_CTX, sgs_VarObj* obj )
{
	int i, j;
	M_HDR;
	if( M->inh.vertexDecl )
		sgs_ObjGCMark( C, M->inh.vertexDecl );
	for( i = 0; i < M->inh.numParts; ++i )
	{
		for( j = 0; j < SS3D_NUM_MATERIAL_TEXTURES; ++j )
			if( M->inh.parts[ i ].textures[ j ] )
				sgs_ObjGCMark( C, M->inh.parts[ i ].textures[ j ] );
		for( j = 0; j < SS3D_MAX_NUM_PASSES; ++j )
			if( M->inh.parts[ i ].shaders[ j ] )
				sgs_ObjGCMark( C, M->inh.parts[ i ].shaders[ j ] );
	}
	return SGS_SUCCESS;
}

static int mesh_vb_init( SGS_CTX, SS3D_Mesh_D3D9* M, sgs_SizeVal size )
{
	int dyn = !!( M->inh.dataFlags & SS3D_MDF_DYNAMIC );
	SAFE_RELEASE( M->VB );
	D3DCALL_( ((SS3D_RD3D9*)M->inh.renderer)->device, CreateVertexBuffer, size, dyn ? D3DUSAGE_DYNAMIC : 0, 0, dyn ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &M->VB, NULL );
	if( !M->VB )
		return sgs_Msg( C, SGS_WARNING, "failed to create vertex buffer" );
	M->inh.vertexDataSize = size;
	return 1;
}
static int mesh_ib_init( SGS_CTX, SS3D_Mesh_D3D9* M, sgs_SizeVal size, int i32 )
{
	int dyn = !!( M->inh.dataFlags & SS3D_MDF_DYNAMIC );
	SAFE_RELEASE( M->IB );
	D3DCALL_( ((SS3D_RD3D9*)M->inh.renderer)->device, CreateIndexBuffer, size, dyn ? D3DUSAGE_DYNAMIC : 0, i32 ? D3DFMT_INDEX32 : D3DFMT_INDEX16, dyn ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &M->IB, NULL );
	if( !M->IB )
		return sgs_Msg( C, SGS_WARNING, "failed to create index buffer" );
	M->inh.dataFlags = ( M->inh.dataFlags & ~SS3D_MDF_INDEX_32 ) | ( SS3D_MDF_INDEX_32 * i32 );
	M->inh.indexDataSize = size;
	return 1;
}
static int mesh_vb_upload( SGS_CTX, SS3D_Mesh_D3D9* M, void* data, sgs_SizeVal size )
{
	void* vb_data;
	
	if( FAILED( D3DCALL_( M->VB, Lock, 0, 0, &vb_data, D3DLOCK_DISCARD ) ) )
		return sgs_Msg( C, SGS_WARNING, "failed to lock vertex buffer" );
	
	memcpy( vb_data, data, size );
	
	if( FAILED( D3DCALL( M->VB, Unlock ) ) )
		return sgs_Msg( C, SGS_WARNING, "failed to unlock vertex buffer" );
	
	return 1;
}
static int mesh_ib_upload( SGS_CTX, SS3D_Mesh_D3D9* M, void* data, sgs_SizeVal size )
{
	void* ib_data;
	
	if( FAILED( D3DCALL_( M->IB, Lock, 0, 0, &ib_data, D3DLOCK_DISCARD ) ) )
		return sgs_Msg( C, SGS_WARNING, "failed to lock index buffer" );
	
	memcpy( ib_data, data, size );
	
	if( FAILED( D3DCALL( M->IB, Unlock ) ) )
		return sgs_Msg( C, SGS_WARNING, "failed to unlock index buffer" );
	
	return 1;
}
static void mesh_set_num_parts( SGS_CTX, SS3D_Mesh_D3D9* M, int num )
{
	int j, cnp = M->inh.numParts;
	while( cnp > num )
	{
		for( j = 0; j < SS3D_NUM_MATERIAL_TEXTURES; ++j )
			if( M->inh.parts[ cnp ].textures[ j ] )
				sgs_ObjRelease( C, M->inh.parts[ cnp ].textures[ j ] );
		for( j = 0; j < SS3D_MAX_NUM_PASSES; ++j )
			if( M->inh.parts[ cnp ].shaders[ j ] )
				sgs_ObjRelease( C, M->inh.parts[ cnp ].shaders[ j ] );
		
		memset( M->inh.parts + cnp, 0, sizeof( SS3D_MeshPart ) );
		cnp--;
	}
	M->inh.numParts = num;
}

static int meshd3d9i_setVertexData( SGS_CTX )
{
	/* args: string buffer, obj vdecl, bool tristrip */
	SS3D_VDecl_D3D9* vdecl = NULL;
	char* buf;
	sgs_SizeVal size;
	sgs_Bool tristrip;
	M_IHDR( setVertexData );
	if( !sgs_LoadArgs( C, "m!ob", &buf, &size, SS3D_VDecl_D3D9_iface, &vdecl, &tristrip ) )
		return 0;
	
	if( !mesh_vb_init( C, M, size ) )
		return 0;
	
	if( !mesh_vb_upload( C, M, buf, size ) )
		return 0;
	
	M->inh.dataFlags = ( M->inh.dataFlags & ~SS3D_MDF_TRIANGLESTRIP ) | ( SS3D_MDF_TRIANGLESTRIP * tristrip );
	M->inh.vertexCount = size / vdecl->info.size;
	sgs_ObjAssign( C, &M->inh.vertexDecl, sgs_GetObjectStruct( C, 1 ) );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_setIndexData( SGS_CTX )
{
	/* args: string buffer, bool i32 */
	char* buf;
	sgs_SizeVal size;
	sgs_Bool i32;
	M_IHDR( setIndexData );
	if( !sgs_LoadArgs( C, "mb", &buf, &size, &i32 ) )
		return 0;
	
	if( !mesh_ib_init( C, M, size, i32 ) )
		return 0;
	
	if( !mesh_ib_upload( C, M, buf, size ) )
		return 0;
	
	M->inh.indexCount = size / ( i32 ? 4 : 2 );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_initVertexBuffer( SGS_CTX )
{
	/* args: int size */
	sgs_Int size;
	M_IHDR( initVertexBuffer );
	if( !sgs_LoadArgs( C, "i", &size ) )
		return 0;
	
	if( !mesh_vb_init( C, M, size ) )
		return 0;
	
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_initIndexBuffer( SGS_CTX )
{
	/* args: int size */
	sgs_Int size;
	sgs_Bool i32;
	M_IHDR( initIndexBuffer );
	if( !sgs_LoadArgs( C, "ib", &size, &i32 ) )
		return 0;
	
	if( !mesh_ib_init( C, M, size, i32 ) )
		return 0;
	
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_updateVertexData( SGS_CTX )
{
	/* args: string buffer, obj vdecl, bool tristrip */
	SS3D_VDecl_D3D9* vdecl = NULL;
	char* buf;
	sgs_SizeVal size;
	sgs_Bool tristrip;
	M_IHDR( updateVertexData );
	if( !sgs_LoadArgs( C, "m!ob", &buf, &size, SS3D_VDecl_D3D9_iface, &vdecl, &tristrip ) )
		return 0;
	
	if( !M->VB )
		return sgs_Msg( C, SGS_WARNING, "vertex buffer is not initialized" );
	
	if( size > M->inh.vertexDataSize )
		return sgs_Msg( C, SGS_WARNING, "vertex data too big (%d > %d)", (int) size, (int) M->inh.vertexDataSize );
	
	if( !mesh_vb_upload( C, M, buf, size ) )
		return 0;
	
	M->inh.dataFlags = ( M->inh.dataFlags & ~SS3D_MDF_TRIANGLESTRIP ) | ( SS3D_MDF_TRIANGLESTRIP * tristrip );
	M->inh.vertexCount = size / vdecl->info.size;
	sgs_ObjAssign( C, &M->inh.vertexDecl, sgs_GetObjectStruct( C, 1 ) );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_updateIndexData( SGS_CTX )
{
	/* args: string buffer */
	char* buf;
	sgs_SizeVal size;
	sgs_Bool i32;
	M_IHDR( updateIndexData );
	if( !sgs_LoadArgs( C, "m", &buf, &size ) )
		return 0;
	
	if( !M->IB )
		return sgs_Msg( C, SGS_WARNING, "index buffer is not initialized" );
	
	if( size > M->inh.indexDataSize )
		return sgs_Msg( C, SGS_WARNING, "index data too big (%d > %d)", (int) size, (int) M->inh.indexDataSize );
	
	if( !mesh_ib_upload( C, M, buf, size ) )
		return 0;
	
	i32 = !!( M->inh.dataFlags & SS3D_MDF_INDEX_32 );
	M->inh.indexCount = size / ( i32 ? 4 : 2 );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_setAABBFromVertexData( SGS_CTX )
{
	SS3D_VDeclInfo* vdinfo = NULL;
	char* buf;
	sgs_SizeVal size;
	M_IHDR( setAABBFromVertexData );
	if( !sgs_LoadArgs( C, "m!o", &buf, &size, SS3D_VDecl_D3D9_iface, &vdinfo ) )
		return 0;
	if( SS3D_GetAABBFromVertexData( vdinfo, buf, size, M->inh.boundsMin, M->inh.boundsMax ) )
	{
		sgs_PushBool( C, 1 );
		return 1;
	}
	return 0;
}

static int meshd3d9i_setPartRanges( SGS_CTX )
{
	sgs_Int pid, vo, vc, io, ic;
	M_IHDR( setPartRanges );
	if( !sgs_LoadArgs( C, "iiiii", &pid, &vo, &vc, &io, &ic ) )
		return 0;
	
	if( pid < 0 || pid >= M->inh.numParts )
		return sgs_Msg( C, SGS_WARNING, "part %d is not made available", (int) pid );
	
	M->inh.parts[ pid ].vertexOffset = vo;
	M->inh.parts[ pid ].vertexCount = vc;
	M->inh.parts[ pid ].indexOffset = io;
	M->inh.parts[ pid ].indexCount = ic;
	SGS_RETURN_BOOL( 1 )
}


static void mesh_set_part_shader( SS3D_Mesh_D3D9* M, int pid, char* shader )
{
	int i;
	char buf[ 140 ];
	SGS_CTX = M->inh.renderer->C;
	
	for( i = 0; i < SS3D_MAX_NUM_PASSES; ++i )
		sgs_ObjAssign( C, &M->inh.parts[ pid ].shaders[ i ], NULL );
	
	strcpy( M->inh.parts[ pid ].shader_name, shader );
	
	memset( buf, 0, sizeof(buf) );
	strcpy( buf, "mtl:" );
	for( i = 0; i < M->inh.renderer->numPasses; ++i )
	{
		int type = M->inh.renderer->passes[ i ].type;
		if( type != SS3D_RPT_OBJECT && type != SS3D_RPT_SHADOWS )
			continue;
		
		strcpy( buf + 4, shader );
		strcat( buf, ":" );
		strcat( buf, M->inh.renderer->passes[ i ].shname );
		
		sgs_ObjAssign( C, &M->inh.parts[ pid ].shaders[ i ], get_shader_obj( (SS3D_RD3D9*) M->inh.renderer, buf ) );
	}
}

static int meshd3d9i_setPartShader( SGS_CTX )
{
	sgs_Int pid;
	char* str;
	M_IHDR( setPartShader );
	if( !sgs_LoadArgs( C, "is", &pid, &str ) )
		return 0;
	
	if( pid < 0 || pid >= M->inh.numParts )
		return sgs_Msg( C, SGS_WARNING, "part %d is not made available", (int) pid );
	if( strlen( str ) > 63 )
		return sgs_Msg( C, SGS_WARNING, "shader name too long: %s", str );
	
	mesh_set_part_shader( M, pid, str );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_setPartTexture( SGS_CTX )
{
	sgs_Int pid, index;
	M_IHDR( setPartTexture );
	if( !sgs_LoadArgs( C, "ii", &pid, &index ) )
		return 0;
	if( sgs_ItemType( C, 2 ) != 0 && !sgs_IsObject( C, 2, M->inh.renderer->ifTexture ) && !sgs_IsObject( C, 2, M->inh.renderer->ifRT ) )
		return sgs_ArgErrorExt( C, 2, 0, "texture", "" );
	
	if( pid < 0 || pid >= M->inh.numParts )
		return sgs_Msg( C, SGS_WARNING, "part %d is not made available", (int) pid );
	if( index < 0 || index >= SS3D_NUM_MATERIAL_TEXTURES )
		return sgs_Msg( C, SGS_WARNING, "texture index %d not available", (int) index );
	
	sgs_ObjAssign( C, &M->inh.parts[ pid ].textures[ index ], sgs_ItemType( C, 2 ) == SGS_VT_OBJECT ? sgs_GetObjectStruct( C, 2 ) : NULL );
	SGS_RETURN_BOOL( 1 )
}

static int meshd3d9i_loadFromBuffer( SGS_CTX )
{
	int p, t;
	char* buf;
	sgs_SizeVal size;
	SS3D_MeshFileData mfd;
	
	M_IHDR( loadFromBuffer );
	if( !sgs_LoadArgs( C, "m", &buf, &size ) )
		return 0;
	
	if( !SS3D_MeshData_Parse( buf, size, &mfd ) )
		return sgs_Msg( C, SGS_WARNING, "could not parse mesh data" );
	
	/* load core mesh data */
	sgs_PushObjectPtr( C, M->inh.renderer->_myobj );
	sgs_PushStringBuf( C, mfd.formatData, mfd.formatSize );
	if( SGS_FAILED( sgs_GlobalCall( C, "SS3D_MeshLoad_GetVertexDecl", 2, 1 ) ) )
		return sgs_Msg( C, SGS_WARNING, "failed to call SS3D_MeshLoad_GetVertexDecl" );
	if( !sgs_IsObject( C, -1, SS3D_VDecl_D3D9_iface ) )
		return sgs_Msg( C, SGS_WARNING, "failed to parse vertex declaration" );
	
	sgs_ObjAssign( C, &M->inh.vertexDecl, sgs_GetObjectStruct( C, -1 ) );
	sgs_Pop( C, 1 );
	M->inh.dataFlags = ( mfd.dataFlags & SS3D_MDF__PUBFLAGMASK ) | SS3D_MDF__PUBFLAGBASE;
	
	mesh_vb_init( C, M, mfd.vertexDataSize );
	mesh_vb_upload( C, M, mfd.vertexData, mfd.vertexDataSize );
	
	mesh_ib_init( C, M, mfd.indexDataSize, !!( mfd.dataFlags & SS3D_MDF_INDEX_32 ) );
	mesh_ib_upload( C, M, mfd.indexData, mfd.indexDataSize );
	
	/* load part data */
	mesh_set_num_parts( C, M, mfd.numParts ); /* count (should be) already validated */
	for( p = 0; p < M->inh.numParts; ++p )
	{
		SS3D_MeshFilePartData* mfdp = mfd.parts + p;
		
		char ntbuf[ 64 ];
		if( mfdp->materialStringSizes[0] > 63 )
		{
			memcpy( ntbuf, mfdp->materialStrings[0], 63 );
			ntbuf[ 63 ] = 0;
		}
		else
		{
			memcpy( ntbuf, mfdp->materialStrings[0], mfdp->materialStringSizes[0] );
			ntbuf[ mfdp->materialStringSizes[0] ] = 0;
		}
		mesh_set_part_shader( M, p, ntbuf );
		
		for( t = 1; t <= 8; ++t )
		{
			if( mfdp->materialStrings[t] )
			{
				sgs_PushObjectPtr( C, M->inh.renderer->_myobj );
				sgs_PushStringBuf( C, mfdp->materialStrings[t], mfdp->materialStringSizes[t] );
				if( SGS_FAILED( sgs_GlobalCall( C, "SS3D_MeshLoad_GetTexture", 2, 1 ) ) )
					return sgs_Msg( C, SGS_WARNING, "failed to call SS3D_MeshLoad_GetTexture" );
				if( !sgs_IsObject( C, -1, SS3D_Texture_D3D9_iface ) )
					return sgs_Msg( C, SGS_WARNING, "failed to load texture '%.*s'", mfdp->materialStringSizes[t], mfdp->materialStrings[t] );
				sgs_ObjAssign( C, &M->inh.parts[ p ].textures[ t - 1 ], sgs_GetObjectStruct( C, -1 ) );
				sgs_Pop( C, 1 );
			}
			else
				sgs_ObjAssign( C, &M->inh.parts[ p ].textures[ t - 1 ], NULL );
		}
		
		M->inh.parts[ p ].vertexOffset = mfdp->vertexOffset;
		M->inh.parts[ p ].vertexCount = mfdp->vertexCount;
		M->inh.parts[ p ].indexOffset = mfdp->indexOffset;
		M->inh.parts[ p ].indexCount = mfdp->indexCount;
	}
	
	memcpy( &M->inh.boundsMin, &mfd.boundsMin, sizeof(mfd.boundsMin) );
	memcpy( &M->inh.boundsMax, &mfd.boundsMax, sizeof(mfd.boundsMax) );
	memcpy( &M->inh.center, &mfd.center, sizeof(mfd.center) );
	M->inh.radius = mfd.radius;
	
	return 1;
}

static int meshd3d9_getindex( SGS_ARGS_GETINDEXFUNC )
{
	M_HDR;
	SGS_BEGIN_INDEXFUNC
		SGS_CASE( "numParts" )         SGS_RETURN_INT( M->inh.numParts )
		SGS_CASE( "useTriStrips" )     SGS_RETURN_BOOL( !!( M->inh.dataFlags & SS3D_MDF_TRIANGLESTRIP ) )
		SGS_CASE( "useI32" )           SGS_RETURN_BOOL( !!( M->inh.dataFlags & SS3D_MDF_INDEX_32 ) )
		SGS_CASE( "isDynamic" )        SGS_RETURN_BOOL( !!( M->inh.dataFlags & SS3D_MDF_DYNAMIC ) )
		SGS_CASE( "transparent" )      SGS_RETURN_BOOL( !!( M->inh.dataFlags & SS3D_MDF_TRANSPARENT ) )
		SGS_CASE( "vertexCount" )      SGS_RETURN_INT( M->inh.vertexCount )
		SGS_CASE( "vertexDataSize" )   SGS_RETURN_INT( M->inh.vertexDataSize )
		SGS_CASE( "indexCount" )       SGS_RETURN_INT( M->inh.indexCount )
		SGS_CASE( "indexDataSize" )    SGS_RETURN_INT( M->inh.indexDataSize )
		
		SGS_CASE( "boundsMin" )        SGS_RETURN_VEC3P( M->inh.boundsMin )
		SGS_CASE( "boundsMax" )        SGS_RETURN_VEC3P( M->inh.boundsMax )
		SGS_CASE( "center" )           SGS_RETURN_VEC3P( M->inh.center )
		SGS_CASE( "radius" )           SGS_RETURN_REAL( M->inh.radius )
		
		SGS_CASE( "setVertexData" )    SGS_RETURN_CFUNC( meshd3d9i_setVertexData )
		SGS_CASE( "setIndexData" )     SGS_RETURN_CFUNC( meshd3d9i_setIndexData )
		SGS_CASE( "setAABBFromVertexData" ) SGS_RETURN_CFUNC( meshd3d9i_setAABBFromVertexData )
		SGS_CASE( "initVertexBuffer" ) SGS_RETURN_CFUNC( meshd3d9i_initVertexBuffer )
		SGS_CASE( "initIndexBuffer" )  SGS_RETURN_CFUNC( meshd3d9i_initIndexBuffer )
		SGS_CASE( "updateVertexData" ) SGS_RETURN_CFUNC( meshd3d9i_updateVertexData )
		SGS_CASE( "updateIndexData" )  SGS_RETURN_CFUNC( meshd3d9i_updateIndexData )
		SGS_CASE( "setPartRanges" )    SGS_RETURN_CFUNC( meshd3d9i_setPartRanges )
		SGS_CASE( "setPartShader" )    SGS_RETURN_CFUNC( meshd3d9i_setPartShader )
		SGS_CASE( "setPartTexture" )   SGS_RETURN_CFUNC( meshd3d9i_setPartTexture )
		SGS_CASE( "loadFromBuffer" )   SGS_RETURN_CFUNC( meshd3d9i_loadFromBuffer )
	SGS_END_INDEXFUNC;
}

static int meshd3d9_setindex( SGS_ARGS_SETINDEXFUNC )
{
	M_HDR;
	SGS_BEGIN_INDEXFUNC
		SGS_CASE( "numParts" )
		{
			mesh_set_num_parts( C, M, (int) sgs_GetIntP( C, val ) );
			return SGS_SUCCESS;
		}
		SGS_CASE( "transparent" )
		{
			M->inh.dataFlags = ( M->inh.dataFlags & ~SS3D_MDF_TRANSPARENT ) | ( SS3D_MDF_TRANSPARENT * sgs_GetBoolP( C, val ) );
			return SGS_SUCCESS;
		}
		
		SGS_CASE( "boundsMin" ) SGS_PARSE_VEC3( M->inh.boundsMin, 0 )
		SGS_CASE( "boundsMax" ) SGS_PARSE_VEC3( M->inh.boundsMax, 0 )
		SGS_CASE( "center" )    SGS_PARSE_VEC3( M->inh.center, 0 )
		SGS_CASE( "radius" )    SGS_PARSE_REAL( M->inh.radius )
	SGS_END_INDEXFUNC;
}

static sgs_ObjInterface SS3D_Mesh_D3D9_iface[1] =
{{
	"SS3D_Mesh_D3D9",
	meshd3d9_destruct, meshd3d9_gcmark,
	meshd3d9_getindex, meshd3d9_setindex,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};

static int create_mesh( SS3D_RD3D9* R, int dynamic )
{
	SS3D_Mesh_D3D9* M = (SS3D_Mesh_D3D9*) sgs_PushObjectIPA( R->inh.C, sizeof(*M), SS3D_Mesh_D3D9_iface );
	SS3D_Mesh_Init( &M->inh );
	M->inh.renderer = &R->inh;
	M->inh.dataFlags |= dynamic;
	SS3D_Renderer_PokeResource( &R->inh, sgs_GetObjectStruct( R->inh.C, -1 ), 1 );
	M->VB = NULL;
	M->IB = NULL;
	return 1;
}


//
//  R E N D E R E R
//

SGS_DECLARE sgs_ObjInterface SS3D_Renderer_D3D9_iface[1];

#define R_HDR SS3D_RD3D9* R = (SS3D_RD3D9*) data->data;
#define R_IHDR( funcname ) SS3D_RD3D9* R; \
	if( !SGS_PARSE_METHOD( C, SS3D_Renderer_D3D9_iface, R, SS3D_Renderer_D3D9, funcname ) ) return 0;

static int rd3d9i_update( SGS_CTX )
{
	float dt;
	R_IHDR( update );
	if( !sgs_LoadArgs( C, "f", &dt ) )
		return 0;
	SS3D_Renderer_Update( &R->inh, dt );
	return 0;
}

#if 0
typedef struct _testVtx
{
	float x, y, z;
	float nx, ny, nz;
	uint32_t col;
	float tx, ty;
}
testVtx;

#define TC0 0.001f
#define TC1 0.999f
static testVtx testVertices[] =
{
	{ -5, -5, -5, -1, -1, -1, D3DCOLOR_XRGB( 233, 222, 211 ), TC0, TC0 },
	{ +5, -5, -5, +1, -1, -1, D3DCOLOR_XRGB( 233, 211, 222 ), TC1, TC0 },
	{ +5, +5, -5, +1, +1, -1, D3DCOLOR_XRGB( 222, 211, 222 ), TC1, TC1 },
	{ -5, +5, -5, -1, +1, -1, D3DCOLOR_XRGB( 222, 222, 211 ), TC0, TC1 },
	{ -5, -5, +5, -1, -1, +1, D3DCOLOR_XRGB( 211, 222, 233 ), TC0, TC0 },
	{ +5, -5, +5, +1, -1, +1, D3DCOLOR_XRGB( 222, 211, 233 ), TC1, TC0 },
	{ +5, +5, +5, +1, +1, +1, D3DCOLOR_XRGB( 222, 211, 233 ), TC1, TC1 },
	{ -5, +5, +5, -1, +1, +1, D3DCOLOR_XRGB( 211, 222, 233 ), TC0, TC1 },
	
	{ -50, -50, 0, 0, 0, +1, D3DCOLOR_XRGB( 233, 222, 211 ), 0, 0 },
	{ +50, -50, 0, 0, 0, +1, D3DCOLOR_XRGB( 233, 211, 222 ), 1, 0 },
	{ +50, +50, 0, 0, 0, +1, D3DCOLOR_XRGB( 222, 211, 222 ), 1, 1 },
	{ -50, +50, 0, 0, 0, +1, D3DCOLOR_XRGB( 222, 222, 211 ), 0, 1 },
};
static uint16_t testIndices[] =
{
	// plane
	8,9,10,
	10,11,8,
	// front
	0, 1, 2,
	2, 3, 0,
	// top
	3, 2, 6,
	6, 7, 3,
	// back
	7, 6, 5,
	5, 4, 7,
	// bottom
	4, 5, 1,
	1, 0, 4,
	// left
	4, 0, 3,
	3, 7, 4,
	// right
	1, 5, 6,
	6, 2, 1,
};
#endif

typedef struct _ssvtx
{
	float x, y, z;
	float u0, v0;
	float u1, v1;
}
ssvtx;

typedef struct _rtoutinfo
{
	IDirect3DSurface9* CS;
	IDirect3DSurface9* DS;
	IDirect3DSurface9* DSS;
	int w, h;
}
rtoutinfo;

static void postproc_blit( SS3D_RD3D9* R, rtoutinfo* pRTOUT, int ds, int ppdata_location )
{
	int w = pRTOUT->w, h = pRTOUT->h;
	
	/* assuming these are validated: */
	SS3D_Scene* scene = (SS3D_Scene*) R->inh.currentScene->data;
	SS3D_Camera* cam = (SS3D_Camera*) scene->camera->data;
	
	float invQW = 2.0f, invQH = 2.0f;
	if( R->inh.viewport )
	{
		SS3D_Viewport* VP = (SS3D_Viewport*) R->inh.viewport->data;
		D3DVIEWPORT9 d3dvp = { VP->x1 / ds, VP->y1 / ds, ( VP->x2 - VP->x1 ) / ds, ( VP->y2 - VP->y1 ) / ds, 0.0f, 1.0f };
		invQW = w * 2.0f / ( VP->x2 - VP->x1 );
		invQH = h * 2.0f / ( VP->y2 - VP->y1 );
		D3DCALL_( R->device, SetViewport, &d3dvp );
	}
	
	w /= ds;
	h /= ds;
	
	float hpox = 0.5f / w;
	float hpoy = 0.5f / h;
	float fsx = 1/cam->mProj[0][0];
	float fsy = 1/cam->mProj[1][1];
	ssvtx ssVertices[] =
	{
		{ -1, -1, 0, 0+hpox, 1+hpoy, -fsx, -fsy },
		{ invQW - 1, -1, 0, 1+hpox, 1+hpoy, +fsx, -fsy },
		{ invQW - 1, invQH - 1, 0, 1+hpox, 0+hpoy, +fsx, +fsy },
		{ -1, invQH - 1, 0, 0+hpox, 0+hpoy, -fsx, +fsy },
	};
	
	if( ppdata_location >= 0 )
	{
		VEC4 ppdata;
		VEC4_Set( ppdata, w, h, 1.0f / w, 1.0f / h );
		pshc_set_vec4array( R, ppdata_location, ppdata, 1 );
	}
	
	D3DCALL_( R->device, SetFVF, D3DFVF_XYZ | D3DFVF_TEX2 );
	D3DCALL_( R->device, DrawPrimitiveUP, D3DPT_TRIANGLEFAN, 2, ssVertices, sizeof(*ssVertices) );
	R->inh.stat_numDrawCalls++;
	R->inh.stat_numPDrawCalls++;
}

static int sort_meshinstlight_by_light( const void* p1, const void* p2 )
{
	SS3D_MeshInstLight* mil1 = (SS3D_MeshInstLight*) p1;
	SS3D_MeshInstLight* mil2 = (SS3D_MeshInstLight*) p2;
	return mil1->L == mil2->L ? 0 : ( mil1->L < mil2->L ? -1 : 1 );
}

static int rd3d9i_render( SGS_CTX )
{
	int i, inst_id, pass_id, light_id, part_id, tex_id;
	R_IHDR( render );
	
	if( !R->inh.currentScene )
		return 0;
	SS3D_Scene* scene = (SS3D_Scene*) R->inh.currentScene->data;
	
	if( !scene->camera )
		return 0;
	SS3D_Camera* cam = (SS3D_Camera*) scene->camera->data;
	
	IDirect3DTexture9* tx_depth = R->drd.RTT_DEPTH;
	IDirect3DSurface9* su_depth = R->drd.RTS_DEPTH;
	rtoutinfo RTOUT = { R->bb_color, su_depth, R->bb_depth, R->inh.width, R->inh.height };
	if( R->inh.currentRT )
	{
		SS3D_RenderTexture_D3D9* RT = (SS3D_RenderTexture_D3D9*) R->inh.currentRT->data;
		RTOUT.CS = RT->CS;
		RTOUT.DS = RT->DS;
		RTOUT.DSS = RT->DSS;
		RTOUT.w = RT->inh.inh.info.width;
		RTOUT.h = RT->inh.inh.info.height;
		tx_depth = RT->DT;
		su_depth = RT->DS;
	}
	int w = RTOUT.w, h = RTOUT.h;
	
	
	SS3D_Texture_D3D9* tx_cubemap = get_texture( R, "testdata/cubemap_pbr.dds" );
	
	
	SS3D_Shader_D3D9* sh_post_process = get_shader( R, "testFRpost" );
	SS3D_Shader_D3D9* sh_post_dshp = get_shader( R, "pp_bloom_dshp" );
	SS3D_Shader_D3D9* sh_post_blur_h = get_shader( R, "pp_bloom_blur_h" );
	SS3D_Shader_D3D9* sh_post_blur_v = get_shader( R, "pp_bloom_blur_v" );
	
	
	/* CULL */
	sgs_MemBuf visible_mesh_buf = sgs_membuf_create();
	uint32_t visible_mesh_count = SS3D_Scene_Cull_Camera_MeshList( C, &visible_mesh_buf, scene );
	SS3D_MeshInstance** visible_meshes = (SS3D_MeshInstance**) visible_mesh_buf.ptr;
	
	sgs_MemBuf visible_point_light_buf = sgs_membuf_create();
	uint32_t visible_point_light_count = SS3D_Scene_Cull_Camera_PointLightList( C, &visible_point_light_buf, scene );
	SS3D_Light** visible_point_lights = (SS3D_Light**) visible_point_light_buf.ptr;
	
	R->inh.stat_numVisMeshes = visible_mesh_count;
	R->inh.stat_numVisPLights = visible_point_light_count;
	R->inh.stat_numVisSLights = 0;
	R->inh.stat_numDrawCalls = 0;
	R->inh.stat_numSDrawCalls = 0;
	R->inh.stat_numMDrawCalls = 0;
	R->inh.stat_numPDrawCalls = 0;
	
	/* SORT OUT MESH INSTANCE / LIGHT RELATIONS */
	sgs_MemBuf inst_light_buf = sgs_membuf_create();
	sgs_MemBuf light_inst_buf = sgs_membuf_create();
	for( inst_id = 0; inst_id < scene->meshInstances.size; ++inst_id )
	{
		SS3D_MeshInstance* MI = (SS3D_MeshInstance*) scene->meshInstances.vars[ inst_id ].val.data.O->data;
		if( !MI->mesh || !MI->enabled )
			continue;
		MI->lightbuf_begin = (SS3D_MeshInstLight*) inst_light_buf.size;
		// POINT LIGHTS
		for( light_id = 0; light_id < visible_point_light_count; ++light_id )
		{
			SS3D_Light* L = visible_point_lights[ light_id ];
			SS3D_MeshInstLight mil = { MI, L };
			sgs_membuf_appbuf( &inst_light_buf, C, &mil, sizeof(mil) );
		}
		// SPOTLIGHTS
		for( light_id = 0; light_id < scene->lights.size; ++light_id )
		{
			SS3D_Light* L = (SS3D_Light*) scene->lights.vars[ light_id ].val.data.O->data;
			if( !L->isEnabled || L->type != SS3DLIGHT_SPOT )
				continue;
			SS3D_MeshInstLight mil = { MI, L };
			sgs_membuf_appbuf( &inst_light_buf, C, &mil, sizeof(mil) );
		}
		MI->lightbuf_end = (SS3D_MeshInstLight*) inst_light_buf.size;
	}
	for( inst_id = 0; inst_id < scene->meshInstances.size; ++inst_id )
	{
		SS3D_MeshInstance* MI = (SS3D_MeshInstance*) scene->meshInstances.vars[ inst_id ].val.data.O->data;
		if( !MI->mesh || !MI->enabled )
			continue;
		MI->lightbuf_begin = (SS3D_MeshInstLight*)( (size_t) MI->lightbuf_begin + (size_t) inst_light_buf.ptr );
		MI->lightbuf_end = (SS3D_MeshInstLight*)( (size_t) MI->lightbuf_end + (size_t) inst_light_buf.ptr );
	}
	
	/*  insts -> lights  TO  lights -> insts  */
	sgs_membuf_resize( &light_inst_buf, C, inst_light_buf.size );
	memcpy( light_inst_buf.ptr, inst_light_buf.ptr, inst_light_buf.size );
	qsort( light_inst_buf.ptr, light_inst_buf.size / sizeof( SS3D_MeshInstLight ), sizeof( SS3D_MeshInstLight ), sort_meshinstlight_by_light );
	SS3D_MeshInstLight* pmil = (SS3D_MeshInstLight*) light_inst_buf.ptr;
	SS3D_MeshInstLight* pmilend = pmil + ( light_inst_buf.size / sizeof( SS3D_MeshInstLight ) );
	for( light_id = 0; light_id < scene->lights.size; ++light_id )
	{
		SS3D_Light* L = (SS3D_Light*) scene->lights.vars[ light_id ].val.data.O->data;
		if( !L->isEnabled )
			continue;
		L->mibuf_begin = NULL;
		L->mibuf_end = NULL;
	}
	while( pmil < pmilend )
	{
		if( !pmil->L->mibuf_begin )
			pmil->L->mibuf_begin = pmil;
		pmil->L->mibuf_end = pmil + 1;
		pmil++;
	}
	
	
	D3DCALL_( R->device, SetRenderState, D3DRS_ZENABLE, 1 );
	D3DCALL_( R->device, SetRenderState, D3DRS_ZWRITEENABLE, 1 );
	
	/* SHADOW RENDERING */
	D3DCALL_( R->device, SetRenderTarget, 1, NULL );
	D3DCALL_( R->device, SetRenderTarget, 2, NULL );
	for( pass_id = 0; pass_id < R->inh.numPasses; ++pass_id )
	{
		SS3D_RenderPass* pass = R->inh.passes + pass_id;
		
		if( pass->type != SS3D_RPT_SHADOWS )
			continue;
		
		for( light_id = 0; light_id < scene->lights.size; ++light_id )
		{
			MAT4 m_world_view, m_inv_view;
			
			SS3D_Light* L = (SS3D_Light*) scene->lights.vars[ light_id ].val.data.O->data;
			if( !L->isEnabled || !L->shadowTexture )
				continue;
			
			SS3D_RenderTexture_D3D9* RT = (SS3D_RenderTexture_D3D9*) L->shadowTexture->data;
			
			D3DCALL_( R->device, SetRenderTarget, 0, RT->CS );
			D3DCALL_( R->device, SetDepthStencilSurface, RT->DSS );
			D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0 );
			
			vshc_set_mat4( R, 4, L->projMatrix );
			memcpy( m_inv_view, L->viewMatrix, sizeof( m_inv_view ) );
			SS3D_Mtx_Transpose( m_inv_view );
			pshc_set_mat4( R, 0, m_inv_view );
			
			pmil = L->mibuf_begin;
			pmilend = L->mibuf_end;
			for( ; pmil < pmilend; ++pmil )
			{
				SS3D_MeshInstance* MI = pmil->MI;
				if( !MI->mesh || !MI->enabled )
					continue;
				
				SS3D_Mesh_D3D9* M = (SS3D_Mesh_D3D9*) MI->mesh->data;
				SS3D_VDecl_D3D9* VD = (SS3D_VDecl_D3D9*) M->inh.vertexDecl->data;
				
				/* if (transparent & want solid) or (solid & want transparent), skip */
				if( M->inh.dataFlags & SS3D_MDF_TRANSPARENT )
					continue;
				
				SS3D_Mtx_Multiply( m_world_view, MI->matrix, L->viewMatrix );
				vshc_set_mat4( R, 0, m_world_view );
				
				D3DCALL_( R->device, SetVertexDeclaration, VD->VD );
				D3DCALL_( R->device, SetStreamSource, 0, M->VB, 0, VD->info.size );
				D3DCALL_( R->device, SetIndices, M->IB );
				
				for( part_id = 0; part_id < M->inh.numParts; ++part_id )
				{
					SS3D_MeshPart* MP = M->inh.parts + part_id;
					
					use_shader( R, (SS3D_Shader_D3D9*) MP->shaders[ pass_id ]->data );
					for( tex_id = 0; tex_id < SS3D_NUM_MATERIAL_TEXTURES; ++tex_id )
						use_texture( R, tex_id, MP->textures[ tex_id ] ? (SS3D_Texture_D3D9*) MP->textures[ tex_id ]->data : NULL );
					
					if( MP->indexCount < 3 )
						continue;
					D3DCALL_( R->device, DrawIndexedPrimitive,
						M->inh.dataFlags & SS3D_MDF_TRIANGLESTRIP ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST,
						MP->vertexOffset, 0, MP->vertexCount, MP->indexOffset, M->inh.dataFlags & SS3D_MDF_TRIANGLESTRIP ? MP->indexCount - 2 : MP->indexCount / 3 );
					R->inh.stat_numDrawCalls++;
					R->inh.stat_numSDrawCalls++;
				}
			}
		}
	}
	
	/* MAIN RENDERING */
	if( R->inh.disablePostProcessing )
	{
		D3DCALL_( R->device, SetRenderTarget, 0, RTOUT.CS );
		D3DCALL_( R->device, SetRenderTarget, 1, NULL );
		D3DCALL_( R->device, SetRenderTarget, 2, RTOUT.DS );
		D3DCALL_( R->device, SetDepthStencilSurface, RTOUT.DSS );
	}
	else
	{
		D3DCALL_( R->device, SetRenderTarget, 0, R->drd.RTS_OCOL );
		D3DCALL_( R->device, SetRenderTarget, 1, R->drd.RTS_PARM );
		D3DCALL_( R->device, SetRenderTarget, 2, R->drd.RTS_DEPTH );
		D3DCALL_( R->device, SetDepthStencilSurface, R->drd.RTSD );
	}
	
	D3DCALL_( R->device, SetRenderState, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	D3DCALL_( R->device, SetRenderState, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	
	if( R->inh.viewport )
	{
		SS3D_Viewport* VP = (SS3D_Viewport*) R->inh.viewport->data;
		D3DVIEWPORT9 d3dvp = { 0, 0, VP->x2 - VP->x1, VP->y2 - VP->y1, 0.0f, 1.0f };
		D3DCALL_( R->device, SetViewport, &d3dvp );
	}
	
	D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
	
	/*
		=== CONSTANT INFO ===
	- POINT LIGHT: (total size: 2 constants, max. count: 16)
		VEC4 [px, py, pz, radius]
		VEC4 [cr, cg, cb, power]
	- SPOT LIGHT: (total size: 4 constants for each shader, 2 textures, max. count: 4)
		TEXTURES [cookie, shadowmap]
	[pixel shader]:
		VEC4 [px, py, pz, radius]
		VEC4 [cr, cg, cb, power]
		VEC4 [dx, dy, dz, angle]
		VEC4 [ux, uy, uz, -]
	[vertex shader]:
		VEC4x4 [shadow map matrix]
	- AMBIENT + DIRECTIONAL (SUN) LIGHT: (total size: 3 constants)
		VEC4 [ar, ag, ab, -]
		VEC4 [dx, dy, dz, -]
		VEC4 [cr, cg, cb, -]
		TEXTURES [some number of shadowmaps]
	- DIR. AMBIENT DATA: (total size: 6 constants)
		VEC4 colorXP, colorXN, colorYP, colorYN, colorZP, colorZN
	- FOG DATA: (total size: 2 constants)
		VEC4 [cr, cg, cb, min.dst]
		VEC4 [h1, d1, h2, d2] (heights, densities)
	- VERTEX SHADER
		0-3: camera view matrix
		4-7: camera projection matrix
		23: light counts (point, spot)
		24-39: VS spot light data
	- PIXEL SHADER
		0-3: camera inverse view matrix
		4: camera position
		12-13: fog data
		14-19: dir.amb. data
		20-22: dir. light
		23: light counts (point, spot)
		24-39: PS spot light data
		56-87: point light data
	*/
	
	/* upload unchanged data */
	vshc_set_mat4( R, 0, cam->mView );
	vshc_set_mat4( R, 4, cam->mProj );
	pshc_set_mat4( R, 0, cam->mInvView );
	VEC4 campos4 = { cam->position[0], cam->position[1], cam->position[2], 0 };
	pshc_set_vec4array( R, 4, campos4, 1 );
	
	VEC4 fogdata[ 2 ] =
	{
		{ scene->fogColor[0] * scene->fogColor[0], scene->fogColor[1] * scene->fogColor[1], scene->fogColor[2] * scene->fogColor[2], scene->fogHeightFactor },
		{ scene->fogDensity, scene->fogHeightDensity, scene->fogStartHeight, scene->fogMinDist },
	};
	pshc_set_vec4array( R, 12, *fogdata, 2 );
	
	VEC4 dirlight[ 3 ] =
	{
		{ scene->ambientLightColor[0], scene->ambientLightColor[1], scene->ambientLightColor[2], 1 },
		{ -scene->dirLightDir[0], -scene->dirLightDir[1], -scene->dirLightDir[2], 0 },
		{ scene->dirLightColor[0], scene->dirLightColor[1], scene->dirLightColor[2], 1 },
	};
	VEC3_Normalized( dirlight[1], dirlight[1] );
	SS3D_Mtx_TransformNormal( dirlight[1], dirlight[1], cam->mView );
	pshc_set_vec4array( R, 20, *dirlight, 3 );
	
	for( pass_id = 0; pass_id < R->inh.numPasses; ++pass_id )
	{
		SS3D_RenderPass* pass = R->inh.passes + pass_id;
		
		if( pass->type == SS3D_RPT_OBJECT )
		{
			int obj_type = !!( pass->flags & SS3D_RPF_OBJ_STATIC ) - !!( pass->flags & SS3D_RPF_OBJ_DYNAMIC );
			int mtl_type = !!( pass->flags & SS3D_RPF_MTL_SOLID ) - !!( pass->flags & SS3D_RPF_MTL_TRANSPARENT );
			
			D3DCALL_( R->device, SetRenderState, D3DRS_ZWRITEENABLE, mtl_type >= 0 );
			D3DCALL_( R->device, SetRenderState, D3DRS_ALPHABLENDENABLE, mtl_type <= 0 );
			
			for( inst_id = 0; inst_id < visible_mesh_count; ++inst_id )
			{
				SS3D_MeshInstance* MI = visible_meshes[ inst_id ];
				
				VEC4 lightdata[ 64 ];
				float *pldata_it = lightdata[0], *sldata_ps_it = lightdata[32], *sldata_vs_it = lightdata[48];
				int pl_count = 0, sl_count = 0;
				
				MAT4 m_world_view;
				
				SS3D_Mesh_D3D9* M = (SS3D_Mesh_D3D9*) MI->mesh->data;
				SS3D_VDecl_D3D9* VD = (SS3D_VDecl_D3D9*) M->inh.vertexDecl->data;
				
				/* if (transparent & want solid) or (solid & want transparent), skip */
				if( ( ( M->inh.dataFlags & SS3D_MDF_TRANSPARENT ) && mtl_type > 0 ) || ( !( M->inh.dataFlags & SS3D_MDF_TRANSPARENT ) && mtl_type < 0 ) )
					continue;
				
				/* TODO dynamic meshes */
				if( obj_type < 0 )
					continue; /* DISABLE them for now... */
				
				if( pass->pointlight_count )
				{
					while( pl_count < pass->pointlight_count && MI->lightbuf_begin < MI->lightbuf_end )
					{
						int found = 0;
						SS3D_MeshInstLight* plt = MI->lightbuf_begin;
						while( plt < MI->lightbuf_end )
						{
							if( plt->L->type == SS3DLIGHT_POINT )
							{
								SS3D_Light* light = plt->L;
								
								found = 1;
								
								// copy data
								VEC3 viewpos;
								SS3D_Mtx_TransformPos( viewpos, light->position, cam->mView );
								VEC4 newdata[2] =
								{
									{ viewpos[0], viewpos[1], viewpos[2], light->range },
									{ light->color[0], light->color[1], light->color[2], light->power }
								};
								memcpy( pldata_it, newdata, sizeof(VEC4)*2 );
								pldata_it += 8;
								pl_count++;
								
								// extract light from array
								if( plt > MI->lightbuf_begin )
									*plt = *MI->lightbuf_begin;
								MI->lightbuf_begin++;
								
								break;
							}
							plt++;
						}
						if( !found )
							break;
					}
					pshc_set_vec4array( R, 56, lightdata[0], 2 * pl_count );
				}
				if( pass->spotlight_count )
				{
					while( sl_count < pass->spotlight_count && MI->lightbuf_begin < MI->lightbuf_end )
					{
						int found = 0;
						SS3D_MeshInstLight* plt = MI->lightbuf_begin;
						while( plt < MI->lightbuf_end )
						{
							if( plt->L->type == SS3DLIGHT_SPOT )
							{
								SS3D_Light* light = plt->L;
								
								found = 1;
								
								// copy data
								VEC3 viewpos, viewdir;
								SS3D_Mtx_TransformPos( viewpos, light->position, cam->mView );
								SS3D_Mtx_TransformNormal( viewdir, light->direction, cam->mView );
								VEC3_Normalized( viewdir, viewdir );
								float tszx = 1, tszy = 1;
								if( light->shadowTexture )
								{
									SS3D_Texture_D3D9* tex = (SS3D_Texture_D3D9*) light->shadowTexture->data;
									tszx = tex->inh.info.width;
									tszy = tex->inh.info.height;
								}
								VEC4 newdata[4] =
								{
									{ viewpos[0], viewpos[1], viewpos[2], light->range },
									{ light->color[0], light->color[1], light->color[2], light->power },
									{ viewdir[0], viewdir[1], viewdir[2], DEG2RAD( light->angle ) },
									{ tszx, tszy, 1.0f / tszx, 1.0f / tszy },
								};
								memcpy( sldata_ps_it, newdata, sizeof(VEC4)*4 );
								MAT4 tmp;
								SS3D_Mtx_Multiply( tmp, MI->matrix, light->viewProjMatrix );
								memcpy( sldata_vs_it, tmp, sizeof(MAT4) );
								sldata_ps_it += 16;
								sldata_vs_it += 16;
								
								use_texture( R, 8 + sl_count * 2, light->cookieTexture ? (SS3D_Texture_D3D9*) light->cookieTexture->data : NULL );
								use_texture( R, 8 + sl_count * 2 + 1, light->shadowTexture ? (SS3D_Texture_D3D9*) light->shadowTexture->data : NULL );
								sl_count++;
								
								// extract light from array
								if( plt > MI->lightbuf_begin )
									*plt = *MI->lightbuf_begin;
								MI->lightbuf_begin++;
								
								break;
							}
							plt++;
						}
						if( !found )
							break;
					}
					vshc_set_vec4array( R, 24, lightdata[48], 4 * sl_count );
					pshc_set_vec4array( R, 24, lightdata[32], 4 * sl_count );
				}
				
				if( pass->flags & SS3D_RPF_LIGHTOVERLAY && pl_count + sl_count <= 0 )
					continue;
				
				if( !( pass->flags & SS3D_RPF_LIGHTOVERLAY ) )
				{
					use_texture( R, 8, tx_cubemap );
					D3DCALL_( R->device, SetRenderState, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
				}
				else
				{
					D3DCALL_( R->device, SetRenderState, D3DRS_ALPHABLENDENABLE, TRUE );
					D3DCALL_( R->device, SetRenderState, D3DRS_DESTBLEND, D3DBLEND_ONE );
				}
				
				VEC4 lightcounts = { pl_count, sl_count, 0, 0 };
				vshc_set_vec4array( R, 23, lightcounts, 1 );
				pshc_set_vec4array( R, 23, lightcounts, 1 );
				
				SS3D_Mtx_Multiply( m_world_view, MI->matrix, cam->mView );
				vshc_set_mat4( R, 0, m_world_view );
				
				D3DCALL_( R->device, SetVertexDeclaration, VD->VD );
				D3DCALL_( R->device, SetStreamSource, 0, M->VB, 0, VD->info.size );
				D3DCALL_( R->device, SetIndices, M->IB );
				
				for( part_id = 0; part_id < M->inh.numParts; ++part_id )
				{
					SS3D_MeshPart* MP = M->inh.parts + part_id;
					
					if( !MP->shaders[ pass_id ] )
						continue;
					
					use_shader( R, (SS3D_Shader_D3D9*) MP->shaders[ pass_id ]->data );
					for( tex_id = 0; tex_id < SS3D_NUM_MATERIAL_TEXTURES; ++tex_id )
						use_texture( R, tex_id, MP->textures[ tex_id ] ? (SS3D_Texture_D3D9*) MP->textures[ tex_id ]->data : NULL );
					
					if( MP->indexCount < 3 )
						continue;
					D3DCALL_( R->device, DrawIndexedPrimitive,
						M->inh.dataFlags & SS3D_MDF_TRIANGLESTRIP ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST,
						MP->vertexOffset, 0, MP->vertexCount, MP->indexOffset, M->inh.dataFlags & SS3D_MDF_TRIANGLESTRIP ? MP->indexCount - 2 : MP->indexCount / 3 );
					R->inh.stat_numDrawCalls++;
					R->inh.stat_numMDrawCalls++;
				}
			}
		}
		else if( pass->type == SS3D_RPT_SCREEN )
		{
			D3DCALL_( R->device, SetRenderState, D3DRS_ALPHABLENDENABLE, 1 );
			D3DCALL_( R->device, SetRenderState, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			D3DCALL_( R->device, SetRenderState, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
			D3DCALL_( R->device, SetRenderState, D3DRS_ZENABLE, 0 );
			D3DCALL_( R->device, SetRenderTarget, 1, NULL );
			D3DCALL_( R->device, SetRenderTarget, 2, NULL );
			D3DCALL_( R->device, SetDepthStencilSurface, NULL );
			
			D3DCALL_( R->device, SetTexture, 0, (IDirect3DBaseTexture9*) tx_depth );
			
			use_shader( R, get_shader( R, pass->shname ) );
			pshc_set_vec4array( R, 4, campos4, 1 );
			postproc_blit( R, &RTOUT, 1, -1 );
			
			if( R->inh.disablePostProcessing )
			{
				D3DCALL_( R->device, SetRenderTarget, 2, RTOUT.DS );
				D3DCALL_( R->device, SetDepthStencilSurface, RTOUT.DSS );
			}
			else
			{
				D3DCALL_( R->device, SetRenderTarget, 1, R->drd.RTS_PARM );
				D3DCALL_( R->device, SetRenderTarget, 2, R->drd.RTS_DEPTH );
				D3DCALL_( R->device, SetDepthStencilSurface, R->drd.RTSD );
			}
			
			D3DCALL_( R->device, SetRenderState, D3DRS_ZENABLE, 1 );
		}
		/* shadow passes rendered above */
	}
	
	sgs_membuf_destroy( &inst_light_buf, C );
	sgs_membuf_destroy( &light_inst_buf, C );
	sgs_membuf_destroy( &visible_mesh_buf, C );
	sgs_membuf_destroy( &visible_point_light_buf, C );
	
	
	/* POST-PROCESS & RENDER TO BACKBUFFER */
	D3DCALL_( R->device, SetRenderState, D3DRS_ALPHABLENDENABLE, 0 );
	D3DCALL_( R->device, SetRenderState, D3DRS_ZENABLE, 0 );
	
	if( !R->inh.disablePostProcessing )
	{
		D3DCALL_( R->device, SetRenderTarget, 1, NULL );
		D3DCALL_( R->device, SetRenderTarget, 2, NULL );
		D3DCALL_( R->device, SetDepthStencilSurface, RTOUT.DSS );
		
		D3DCALL_( R->device, SetRenderTarget, 0, R->drd.RTS_BLOOM_DSHP );
		D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0 );
		D3DCALL_( R->device, SetTexture, 0, (IDirect3DBaseTexture9*) R->drd.RTT_OCOL );
		use_shader( R, sh_post_dshp );
		postproc_blit( R, &RTOUT, 4, 0 );
		
		D3DCALL_( R->device, SetRenderTarget, 0, R->drd.RTS_BLOOM_BLUR1 );
		D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0 );
		D3DCALL_( R->device, SetTexture, 0, (IDirect3DBaseTexture9*) R->drd.RTT_BLOOM_DSHP );
		use_shader( R, sh_post_blur_h );
		postproc_blit( R, &RTOUT, 4, 0 );
		
		D3DCALL_( R->device, SetRenderTarget, 0, R->drd.RTS_BLOOM_BLUR2 );
		D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0 );
		D3DCALL_( R->device, SetTexture, 0, (IDirect3DBaseTexture9*) R->drd.RTT_BLOOM_BLUR1 );
		use_shader( R, sh_post_blur_v );
		postproc_blit( R, &RTOUT, 4, 0 );
		
		D3DCALL_( R->device, SetRenderTarget, 0, RTOUT.CS );
	//	D3DCALL_( R->device, Clear, 0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0 );
		use_shader( R, sh_post_process );
		D3DCALL_( R->device, SetTexture, 0, (IDirect3DBaseTexture9*) R->drd.RTT_OCOL );
		D3DCALL_( R->device, SetTexture, 1, (IDirect3DBaseTexture9*) R->drd.RTT_PARM );
		D3DCALL_( R->device, SetTexture, 2, (IDirect3DBaseTexture9*) R->drd.RTT_BLOOM_BLUR2 );
		D3DCALL_( R->device, SetSamplerState, 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		postproc_blit( R, &RTOUT, 1, 0 );
	}
	
	
	D3DCALL_( R->device, SetTexture, 0, NULL );
	use_shader( R, NULL );
	
	if( !R->inh.currentRT )
	{
		RECT srcRect = { 0, 0, w, h };
		RECT srcRect2 = { 0, 0, w/4, h/4 };
		RECT dstRect = { 0, h/8, w/8, h/4 };
		IDirect3DSurface9* surfs[] = {
			R->drd.RTS_OCOL,
			R->drd.RTS_PARM,
	//		R->drd.RTS_BLOOM_DSHP,
	//		R->drd.RTS_BLOOM_BLUR1,
			R->drd.RTS_BLOOM_BLUR2,
		};
		for( i = 0; i < sizeof(surfs)/sizeof(surfs[0]); ++i )
		{
			RECT* sr = ( surfs[i] == R->drd.RTS_BLOOM_DSHP || surfs[i] == R->drd.RTS_BLOOM_BLUR1 || surfs[i] == R->drd.RTS_BLOOM_BLUR2 ) ? &srcRect2 : &srcRect;
			D3DCALL_( R->device, StretchRect, surfs[i], sr, R->bb_color, &dstRect, D3DTEXF_POINT );
			dstRect.left += w/8;
			dstRect.right += w/8;
		}
	}
	
	D3DCALL_( R->device, SetRenderState, D3DRS_ALPHABLENDENABLE, 1 );
	D3DCALL_( R->device, SetRenderState, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	D3DCALL_( R->device, SetRenderState, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	
	return 0;
}

static int rd3d9i_resize( SGS_CTX )
{
	sgs_Int w, h;
	R_IHDR( resize );
	if( !sgs_LoadArgs( C, "ii", &w, &h ) )
		return 0;
	SS3D_Renderer_Resize( &R->inh, w, h );
	return 0;
}

static int rd3d9i_onDeviceLost( SGS_CTX )
{
	R_IHDR( onDeviceLost );
	UNUSED( R );
	return 0;
}

static int rd3d9i_onDeviceReset( SGS_CTX )
{
	R_IHDR( onDeviceReset );
	UNUSED( R );
	return 0;
}

static int rd3d9i_getShader( SGS_CTX )
{
	sgs_Variable key;
	R_IHDR( getShader );
	if( !sgs_LoadArgs( C, "?s<v", &key ) )
		return 0;
	return get_shader_( R, &key );
}

static int rd3d9i_getTexture( SGS_CTX )
{
	sgs_Variable key;
	R_IHDR( getTexture );
	if( !sgs_LoadArgs( C, "?s<v", &key ) )
		return 0;
	return get_texture_( R, &key );
}

static int rd3d9i_createVertexDecl( SGS_CTX )
{
	char* str;
	R_IHDR( createVertexDecl );
	if( !sgs_LoadArgs( C, "s", &str ) )
		return 0;
	return create_vdecl( R, str );
}

static int rd3d9i_createMesh( SGS_CTX )
{
	sgs_Bool dyn = 0;
	R_IHDR( createMesh );
	if( !sgs_LoadArgs( C, "|b", &dyn ) )
		return 0;
	return create_mesh( R, dyn );
}

static int rd3d9i_createRT( SGS_CTX )
{
	sgs_Int w, h, f;
	R_IHDR( createRT );
	if( !sgs_LoadArgs( C, "iii", &w, &h, &f ) )
		return 0;
	return rtd3d9_create( R, (int) w, (int) h, (int) f );
}

static int rd3d9i_createScene( SGS_CTX )
{
	R_IHDR( createScene );
	SS3D_Renderer_PushScene( &R->inh );
	return 1;
}

static int rd3d9_getindex( SGS_CTX, sgs_VarObj* data, sgs_Variable* key, int isprop )
{
	R_HDR;
	SGS_BEGIN_INDEXFUNC
		// properties
		SGS_CASE( "currentScene" )     SGS_RETURN_OBJECT( R->inh.currentScene )
		SGS_CASE( "store" )            SGS_RETURN_OBJECT( R->inh.store )
		SGS_CASE( "currentRT" )        SGS_RETURN_OBJECT( R->inh.currentRT )
		SGS_CASE( "viewport" )         SGS_RETURN_OBJECT( R->inh.viewport )
		
		SGS_CASE( "disablePostProcessing" ) SGS_RETURN_BOOL( R->inh.disablePostProcessing )
		
		SGS_CASE( "stat_numVisMeshes" )  SGS_RETURN_INT( R->inh.stat_numVisMeshes )
		SGS_CASE( "stat_numVisPLights" ) SGS_RETURN_INT( R->inh.stat_numVisPLights )
		SGS_CASE( "stat_numVisSLights" ) SGS_RETURN_INT( R->inh.stat_numVisSLights )
		SGS_CASE( "stat_numDrawCalls" )  SGS_RETURN_INT( R->inh.stat_numDrawCalls )
		SGS_CASE( "stat_numSDrawCalls" ) SGS_RETURN_INT( R->inh.stat_numSDrawCalls )
		SGS_CASE( "stat_numMDrawCalls" ) SGS_RETURN_INT( R->inh.stat_numMDrawCalls )
		SGS_CASE( "stat_numPDrawCalls" ) SGS_RETURN_INT( R->inh.stat_numPDrawCalls )
		
		// methods
		SGS_CASE( "update" )           SGS_RETURN_CFUNC( rd3d9i_update )
		SGS_CASE( "render" )           SGS_RETURN_CFUNC( rd3d9i_render )
		SGS_CASE( "resize" )           SGS_RETURN_CFUNC( rd3d9i_resize )
		SGS_CASE( "onDeviceLost" )     SGS_RETURN_CFUNC( rd3d9i_onDeviceLost )
		SGS_CASE( "onDeviceReset" )    SGS_RETURN_CFUNC( rd3d9i_onDeviceReset )
		
		SGS_CASE( "getShader" )        SGS_RETURN_CFUNC( rd3d9i_getShader )
		SGS_CASE( "getTexture" )       SGS_RETURN_CFUNC( rd3d9i_getTexture )
		SGS_CASE( "createVertexDecl" ) SGS_RETURN_CFUNC( rd3d9i_createVertexDecl )
		SGS_CASE( "createMesh" )       SGS_RETURN_CFUNC( rd3d9i_createMesh )
		SGS_CASE( "createRT" )         SGS_RETURN_CFUNC( rd3d9i_createRT )
		SGS_CASE( "createScene" )      SGS_RETURN_CFUNC( rd3d9i_createScene )
	SGS_END_INDEXFUNC;
}

static int rd3d9_setindex( SGS_CTX, sgs_VarObj* data, sgs_Variable* key, sgs_Variable* val, int isprop )
{
	R_HDR;
	SGS_BEGIN_INDEXFUNC
		SGS_CASE( "currentScene" ) SGS_PARSE_OBJECT_IF( SS3D_Scene_iface, R->inh.currentScene, 0, ((SS3D_Scene*)sgs_GetObjectDataP( val ))->renderer == &R->inh )
		SGS_CASE( "currentRT" )    SGS_PARSE_OBJECT( SS3D_RenderTexture_D3D9_iface, R->inh.currentRT, 0 )
		SGS_CASE( "viewport" )
		{
			if( val->type == SGS_VT_NULL )
			{
				sgs_ObjAssign( C, &R->inh.viewport, NULL );
				return SGS_SUCCESS;
			}
			else if( sgs_IsObjectP( val, SS3D_Viewport_iface ) )
			{
				sgs_ObjAssign( C, &R->inh.viewport, sgs_GetObjectStructP( val ) );
				return SGS_SUCCESS;
			}
			return SGS_EINVAL;
		}
		
		SGS_CASE( "disablePostProcessing" ) SGS_PARSE_BOOL( R->inh.disablePostProcessing )
	SGS_END_INDEXFUNC;
}

static int rd3d9_destruct( SGS_CTX, sgs_VarObj* data )
{
	R_HDR;
	
	postproc_free( R, &R->drd );
	SAFE_RELEASE( R->bb_color );
	SAFE_RELEASE( R->bb_depth );
	SAFE_RELEASE( R->device );
	
	SS3D_Renderer_Destruct( &R->inh );
	
	return SGS_SUCCESS;
}

static sgs_ObjInterface SS3D_Renderer_D3D9_iface[1] =
{{
	"SS3D_Renderer_D3D9",
	rd3d9_destruct, NULL,
	rd3d9_getindex, rd3d9_setindex,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
}};


int SS3D_PushRenderer_D3D9( SGS_CTX, void* device )
{
	sgs_BreakIf( !device );
	SS3D_RD3D9* R = (SS3D_RD3D9*) sgs_PushObjectIPA( C, sizeof(*R), SS3D_Renderer_D3D9_iface );
	
	SS3D_Renderer_Construct( &R->inh, C );
	
	R->inh._myobj = sgs_GetObjectStruct( C, -1 );
	R->inh.API = "D3D9";
	R->inh.ifMesh = SS3D_Mesh_D3D9_iface;
	R->inh.ifTexture = SS3D_Texture_D3D9_iface;
	R->inh.ifShader = SS3D_Shader_D3D9_iface;
	R->inh.ifRT = SS3D_RenderTexture_D3D9_iface;
	
	R->device = (IDirect3DDevice9*) device;
	D3DCALL( R->device, AddRef );
	D3DCALL_( R->device, GetRenderTarget, 0, &R->bb_color );
	D3DCALL_( R->device, GetDepthStencilSurface, &R->bb_depth );
	{
		D3DSURFACE_DESC desc;
		HRESULT hr = D3DCALL_( R->bb_color, GetDesc, &desc );
		sgs_BreakIf( FAILED( hr ) );
		R->inh.width = desc.Width;
		R->inh.height = desc.Height;
	}
	
	postproc_init( R, &R->drd, R->inh.width, R->inh.height );
	
	return 1;
}

