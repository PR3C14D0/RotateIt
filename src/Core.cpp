#include "Core.h"

Core::Core(HWND& hwnd) {
	this->hwnd = hwnd;
	this->rotation = 0.f;
	this->rotSpeed = 0.5f;
	
	DXGI_SWAP_CHAIN_DESC scDesc = { };
	ZeroMemory(&scDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	scDesc.BufferCount = 1;
	scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scDesc.OutputWindow = this->hwnd;
	scDesc.SampleDesc.Count = 4;
	scDesc.Windowed = TRUE;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	
	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		&scDesc,
		this->sc.GetAddressOf(),
		this->dev.GetAddressOf(),
		nullptr,
		this->con.GetAddressOf()
	);

	ID3D11Texture2D* pBackBuffer;
	this->sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	this->dev->CreateRenderTargetView(pBackBuffer, nullptr, this->backBuffer.GetAddressOf());

	RECT rect;
	GetWindowRect(hwnd, &rect);
	this->width = rect.right - rect.left;
	this->height = rect.bottom - rect.top;

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = this->width;
	viewport.Height = this->height;
	
	this->con->OMSetRenderTargets(1, this->backBuffer.GetAddressOf(), nullptr);
	this->con->RSSetViewports(1, &viewport);

	this->InitBuffers();
}

void Core::InitBuffers() {
	this->LoadModel("f16.obj");

	D3D11_BUFFER_DESC posBuffDesc = { };
	ZeroMemory(&posBuffDesc, sizeof(D3D11_BUFFER_DESC));

	posBuffDesc.ByteWidth = sizeof(vertex) * vert.size();
	posBuffDesc.Usage = D3D11_USAGE_DYNAMIC;
	posBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	posBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	
	this->dev->CreateBuffer(&posBuffDesc, nullptr, this->posBuff.GetAddressOf());

	D3D11_MAPPED_SUBRESOURCE ms = { };
	this->con->Map(this->posBuff.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, this->vert.data(), sizeof(vertex) * vert.size());
	this->con->Unmap(this->posBuff.Get(), NULL);

	this->cbuff.Model = XMMatrixTranspose(XMMatrixIdentity());
	this->cbuff.Projection = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XMConvertToRadians(90.f), (float)this->width / (float)this->height, .1f, 300.f));
	this->cbuff.View = XMMatrixTranspose(XMMatrixLookToLH(XMVectorSet(0.f, 0.f, -2.f, 0.f), XMVectorSet(0.f, 0.f, 1.f, 0.f), XMVectorSet(0.f, 1.f, 0.f, 0.f)));

	D3D11_BUFFER_DESC cbufferDesc = { };
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));

	cbufferDesc.ByteWidth = sizeof(cbuff);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	this->dev->CreateBuffer(&cbufferDesc, nullptr, this->cbuffer.GetAddressOf());
	UpdateCBuffers();

	ID3DBlob* VertexShader, *PixelShader;
	ID3DBlob* error;
	D3DX11CompileFromFile(L"shader.fx", nullptr, NULL, "VertexMain", "vs_4_0", NULL, NULL, nullptr, &VertexShader, nullptr, nullptr);
	D3DX11CompileFromFile(L"shader.fx", nullptr, NULL, "PixelMain", "ps_4_0", NULL, NULL, nullptr, &PixelShader, &error, nullptr);

	if (error) {
		MessageBoxA(NULL, (char*)error->GetBufferPointer(), "Error", MB_OK);
		return;
	}

	this->dev->CreateVertexShader(VertexShader->GetBufferPointer(), VertexShader->GetBufferSize(), nullptr, this->vShader.GetAddressOf());
	this->dev->CreatePixelShader(PixelShader->GetBufferPointer(), PixelShader->GetBufferSize(), nullptr, this->pShader.GetAddressOf());

	this->con->VSSetShader(this->vShader.Get(), nullptr, NULL);
	this->con->PSSetShader(this->pShader.Get(), nullptr, NULL);

	D3D11_INPUT_ELEMENT_DESC iLayoutDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, NULL},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, NULL},
	};

	this->dev->CreateInputLayout(iLayoutDesc, 2, VertexShader->GetBufferPointer(), VertexShader->GetBufferSize(), this->iLayout.GetAddressOf());
	this->con->IASetInputLayout(this->iLayout.Get());
	this->con->VSSetConstantBuffers(0, 1, this->cbuffer.GetAddressOf());
}

void Core::UpdateCBuffers() {
	D3D11_MAPPED_SUBRESOURCE ms = { };
	this->rotation += rotSpeed;
	this->cbuff.Model = XMMatrixTranspose(XMMatrixIdentity() * XMMatrixRotationY(XMConvertToRadians(this->rotation)));
	this->con->Map(this->cbuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, &this->cbuff, sizeof(this->cbuff));
	this->con->Unmap(this->cbuffer.Get(), NULL);
}

void Core::LoadModel(string fileName) {
	char filePath[MAX_PATH];
	GetFullPathNameA(fileName.c_str(), MAX_PATH, filePath, nullptr);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, NULL);

	if (scene->HasMeshes()) {
		aiMesh* mesh = scene->mMeshes[0];
		
		for (int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D pos = mesh->mVertices[i];
			aiVector3D texCoords = mesh->mTextureCoords[0][i];
			vertex vert = { pos.x, pos.y, pos.z, texCoords.x, texCoords.y };
			this->vert.push_back(vert);
		}

		aiString texPath;
		scene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

		
	}

	return;
}

void Core::MainLoop() {
	this->con->ClearRenderTargetView(this->backBuffer.Get(), RGBA{ 0.f, 0.f, 0.f, 1.f });

	UINT offset = 0;
	UINT stride = sizeof(vertex);

	this->con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->con->IASetVertexBuffers(0, 1, this->posBuff.GetAddressOf(), &stride, &offset);

	this->UpdateCBuffers();

	this->con->Draw(this->vert.size(), 0);

	this->sc->Present(1, 0);
}