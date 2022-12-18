#pragma once
#include <iostream>
#include <Windows.h>
#include <DirectX/DXGI.h>
#include <DirectX/D3D11.h>
#include <DirectX/D3DX11.h>
#include <DirectX/xnamath.h>
#include <wrl.h>
#include <vector>
#include <Assimp/scene.h>
#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>

using namespace std;
using namespace Microsoft::WRL;
typedef float RGBA[4];

struct ConstantBuffer {
	XMMATRIX Model;
	XMMATRIX View;
	XMMATRIX Projection;
};

struct vertex {
	float x, y, z;
	float r, g;
};

class Core {
private:
	HWND hwnd;
	ComPtr<ID3D11Device> dev;
	ComPtr<ID3D11DeviceContext> con;
	ComPtr<IDXGISwapChain> sc;
	ComPtr<ID3D11RenderTargetView> backBuffer;
	ComPtr<ID3D11Buffer> posBuff;
	
	ComPtr<ID3D11VertexShader> vShader;
	ComPtr<ID3D11PixelShader> pShader;

	ComPtr<ID3D11InputLayout> iLayout;
	ComPtr<ID3D11Buffer> cbuffer;

	ComPtr<ID3D11DepthStencilView> depthBuffer;

	ConstantBuffer cbuff;

	float rotation;
	float rotSpeed;

	int width;
	int height;

	vector<vertex> vert;

	void InitBuffers();
	void LoadModel(string fileName);
	void UpdateCBuffers();
public:
	Core(HWND& hwnd);
	void MainLoop();
};