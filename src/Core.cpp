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
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	ID3D11Texture2D* depthTex;

	D3D11_TEXTURE2D_DESC depthTexDesc = { };
	ZeroMemory(&depthTexDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.SampleDesc.Count = 4;
	depthTexDesc.Height = this->height;
	depthTexDesc.Width = this->width;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	this->dev->CreateTexture2D(&depthTexDesc, nullptr, &depthTex);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = { };
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	dsvDesc.Format = depthTexDesc.Format;

	this->dev->CreateDepthStencilView(depthTex, &dsvDesc, this->depthBuffer.GetAddressOf());

	this->con->OMSetRenderTargets(1, this->backBuffer.GetAddressOf(), this->depthBuffer.Get());
	this->con->RSSetViewports(1, &viewport);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(this->hwnd);
	ImGui_ImplDX11_Init(this->dev.Get(), this->con.Get());

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

	/*this->cbuff.View *= XMMatrixTranspose(XMMatrixRotationX(XMConvertToRadians(45.f)));
	this->cbuff.View *= XMMatrixTranspose(XMMatrixTranslation(0.f, 0.f, 0.f));*/

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
		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 && mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
			HRESULT hr = D3DX11CreateShaderResourceViewFromFileA(this->dev.Get(), texPath.C_Str(), nullptr, nullptr, this->modelTex.GetAddressOf(), nullptr);
			if (FAILED(hr)) {
				MessageBoxA(this->hwnd, "An error occurred while creating shader resource view.", "Error", MB_OK | MB_ICONERROR);
				return;
			}

			D3D11_SAMPLER_DESC samplerDesc = { };
			ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
			samplerDesc.MinLOD = 0;
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

			this->dev->CreateSamplerState(&samplerDesc, this->sampler.GetAddressOf());
			this->con->PSSetShaderResources(0, 1, this->modelTex.GetAddressOf());
			this->con->PSSetSamplers(0, 1, this->sampler.GetAddressOf());
		}
	}

	return;
}

void Core::MainLoop() {
	this->MenuLoop();
	ImGui::Render();
	this->con->ClearRenderTargetView(this->backBuffer.Get(), RGBA{ 0.f, 0.f, 0.f, 1.f });
	this->con->ClearDepthStencilView(this->depthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0.f);

	UINT offset = 0;
	UINT stride = sizeof(vertex);

	this->con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->con->IASetVertexBuffers(0, 1, this->posBuff.GetAddressOf(), &stride, &offset);

	this->UpdateCBuffers();

	this->con->Draw(this->vert.size(), 0);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	this->sc->Present(1, 0);
}

void Core::Cleanup() {
	if (MessageBoxA(this->hwnd, "Are you sure you want to exit?", "Confirmation", MB_OKCANCEL) == IDOK) {
		this->dev->Release();
		this->con->Release();
		this->backBuffer->Release();
		this->depthBuffer->Release();
		this->cbuffer->Release();
		this->modelTex->Release();
		this->sampler->Release();
		this->iLayout->Release();
		this->posBuff->Release();
		this->sc->Release();

		DestroyWindow(hwnd);
	}
}

void Core::MenuLoop() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::BeginMainMenuBar();
	ImGui::MenuItem("Open");
	/* Cleanup */
	if (ImGui::MenuItem("Exit"))
		this->Cleanup();
	ImGui::EndMainMenuBar();

	ImGui::SetNextWindowSize(ImVec2{ 200.f, 600.f, }, NULL);
	ImGui::Begin("Properties", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::Text("Rotation speed");
	ImGui::InputFloat("##btnSpeed", &this->rotSpeed, .5f, 1.f, "%.1f", NULL);
	ImGui::End();

	if (this->rotSpeed > 100.f)
		this->rotSpeed = 100.f;
	if (this->rotSpeed < -100.f)
		this->rotSpeed = -100.f;
}