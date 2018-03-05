//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_11_0),
    m_backBufferIndex(0),
    m_fenceValues{}
{
}

Game::~Game()
{
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<Mouse>();
	m_mouse->SetWindow(window);

    CreateDevice();
    CreateResources();
	m_camera.SetPosition(0.0f, 1.0f, -5.0f);
	m_camera.LookAt(m_camera.GetPosition3f(), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    
	//m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	OnKeyboardInput(timer);
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
	Vector3 shapePos = Vector3(0.0f, 0.0f, cosf(elapsedTime));

	m_world = m_world;

	// update earth rotation
	m_earthRotation = m_earthRotation.CreateRotationY(elapsedTime);

	auto kb = m_keyboard->GetState();
	if (kb.Escape)
		PostQuitMessage(0);

	auto mouse = m_mouse->GetState();


	if (mouse.positionMode == Mouse::MODE_RELATIVE)
	{
		static const float ROTATION_GAIN = 0.004f;

		Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
			* ROTATION_GAIN;

		m_camera.Pitch(delta.y);
		m_camera.RotateY(delta.x);

		
	}

	m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);


}

void Game::OnKeyboardInput(DX::StepTimer const & timer)
{
	
	const float dt = 0.01f; //timer.GetElapsedTicks(); // TODO: Totally calculated

	if (GetAsyncKeyState('W') & 0x8000)
		m_camera.Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_camera.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_camera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_camera.Strafe(10.0f*dt);

	m_camera.UpdateViewMatrix();
}


// Draws the scene.
void Game::Render() //RenderHere
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    Clear();

    // TODO: Add your rendering code here.
	m_spriteBatch->Begin(m_commandList.Get());

	// renderText
	// drawBackgroud
	
	m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background),
		GetTextureSize(m_background.Get()),
		m_fullscreenRect);

	Vector2 fpsPosition(5.0f, 5.0f);
	Vector2 textPosition(5.0f, 25.0f);
	Vector3 camPos = m_camera.GetPosition();
	drawText(std::to_string(m_timer.GetFramesPerSecond()).c_str(), fpsPosition);
	// prepare the camera position string
	std::string camString = "camera(x,y,z): " + std::to_string((float)camPos.x) + ":" + std::to_string((float)camPos.y) + ":" + std::to_string((float)camPos.z);
	drawText(camString.c_str(), textPosition);
	
	m_spriteBatch->End();



	// rendergrid
	m_gridEffect->SetWorld(m_world);
	m_gridEffect->SetView(m_camera.GetView());
	m_gridEffect->SetProjection(m_camera.GetProj());
	

	m_gridEffect->Apply(m_commandList.Get());

	m_batch->Begin(m_commandList.Get());

	Vector3 origin = Vector3::One * sinf(m_timer.GetElapsedSeconds());

	size_t divisions = 10;


	drawGrid(Vector3::UnitX, Vector3::UnitY, origin + Vector3(0.f, 0.f, 1.f), XMFLOAT4(1.f, 0.f, 0.f, 0.01f), divisions);
	drawGrid(Vector3::UnitX, Vector3::UnitZ, origin + Vector3(0.f, -1.f, 0.f), XMFLOAT4(0.f, 1.f, 0.f, 0.01f), divisions);
	drawGrid(Vector3::UnitY, Vector3::UnitZ, origin + Vector3(-1.f, 0.f, 0.f), XMFLOAT4(0.f, 0.f, 1.f, 0.01f), divisions);
	
	m_batch->End();

	// render sphere
	ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	float time = (float)m_timer.GetTotalSeconds();
	Vector3 shapePos = Vector3(cosf(time) * 0.5f, 0.f, sinf(time) * 0.5f);
	//m_shapeEffect->SetMatrices(m_world * m_earthRotation * Matrix::CreateTranslation(shapePos) , m_camera.GetView(), m_camera.GetProj());
	m_shapeEffect->SetMatrices(Matrix::CreateRotationY(time / 2.0f) * Matrix::CreateTranslation(shapePos) * m_world, m_camera.GetView(), m_camera.GetProj());

	/*
	for (auto&& itr : m_renderItems) {
		m_shapeEffect->SetMatrices(m_world * m_rotation * Matrix::CreateTranslation(shapePos), m_view, m_proj);
		m_shapeEffect->Apply(m_commandList.Get());
		itr->Geo->Draw(m_commandList.Get());
	};
	*/
	//m_shapeEffect->SetMatrices(m_world, m_camera.GetView(), m_camera.GetProj());

	m_shapeEffect->Apply(m_commandList.Get());

	m_shape->Draw(m_commandList.Get());
	//m_shape2->Draw(m_commandList.Get());

    // Show the new frame.
    Present();
	m_graphicsMemory->Commit(m_commandQueue.Get());
}

// Helper method to prepare the command list for rendering and clear the back buffers.
void Game::Clear()
{
    // Reset command list and allocator.
    DX::ThrowIfFailed(m_commandAllocators[m_backBufferIndex]->Reset());
    DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_backBufferIndex].Get(), nullptr));

    // Transition the render target into the correct state to allow for drawing into it.
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_offscreenRenderTarget.Get(),
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);


    m_commandList->ResourceBarrier(1, &barrier);

    // Clear the views.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), c_swapBufferCount, m_rtvDescriptorSize);

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptor(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    m_commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    D3D12_RECT scissorRect = { 0, 0, m_outputWidth, m_outputHeight };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);
}

// Submits the command list to the GPU and presents the back buffer contents to the screen.
void Game::Present()
{
	
	D3D12_RESOURCE_BARRIER barriers[2] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenRenderTarget.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RESOLVE_DEST)
	};
	m_commandList->ResourceBarrier(2, barriers);

	m_commandList->ResolveSubresource(m_renderTargets[m_backBufferIndex].Get(), 0,
		m_offscreenRenderTarget.Get(), 0, DXGI_FORMAT_B8G8R8A8_UNORM);


    // Transition the render target to the state that allows it to be presented to the display.
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);
	

    // Send the command list off to the GPU for processing.
    DX::ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);

        MoveToNextFrame();
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();
	
    // TODO: Game window is being resized.
	m_camera.SetLens(0.25f*DirectX::XM_PI, m_outputWidth / m_outputHeight, 1.0f, 1000.0f);
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    DWORD dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
        {
            dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }
#endif

    DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.GetAddressOf());

    // Create the DX12 API device object.
    DX::ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        m_featureLevel,
        IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
        ));

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = c_swapBufferCount + 1; // <---- Add one here!
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[n].ReleaseAndGetAddressOf())));
    }

    // Create a command list for recording graphics commands.
    DX::ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_commandList->Close());

    // Create a fence for tracking GPU execution progress.
    DX::ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fenceValues[m_backBufferIndex]++;

    m_fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!m_fenceEvent.IsValid())
    {
        throw std::exception("CreateEvent");
    }

    // TODO: Initialize device dependent objects here (independent of window size). // CreateDeviceHere



	m_graphicsMemory = std::make_unique<GraphicsMemory>(m_d3dDevice.Get());

	// set render target state
	RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
	rtState.sampleDesc.Count = 4; // <---- 4x MSAA

	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, FALSE,
		D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, TRUE, FALSE,
		0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);


	// effect pipeline state description for grid
	EffectPipelineStateDescription effect_pd(
		&VertexPositionColor::InputLayout,
		CommonStates::AlphaBlend,
		CommonStates::DepthDefault,
		rastDesc,
		rtState,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

	// effect pipeline state descriptr for geometry shape
	EffectPipelineStateDescription shape_pd(
		&GeometricPrimitive::VertexType::InputLayout,
		CommonStates::Opaque,
		CommonStates::DepthDefault,
		CommonStates::CullNone,
		rtState
	);



	m_resourceDescriptors = std::make_unique<DescriptorHeap>(m_d3dDevice.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		Descriptors::Count);

	m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());

	ResourceUploadBatch resourceUpload(m_d3dDevice.Get());

	resourceUpload.Begin(); // ResourceUploadHere

	
	DX::ThrowIfFailed(
		CreateWICTextureFromFile(m_d3dDevice.Get(), resourceUpload, L"galaxy.jpg",
			m_background.ReleaseAndGetAddressOf()));

	DX::ThrowIfFailed(
		CreateWICTextureFromFile(m_d3dDevice.Get(), resourceUpload, L"earth.bmp",
			m_texture.ReleaseAndGetAddressOf(), false));

	CreateShaderResourceView(m_d3dDevice.Get(), m_background.Get(),
		m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

	CreateShaderResourceView(m_d3dDevice.Get(), m_texture.Get(),
		m_resourceDescriptors->GetCpuHandle(Descriptors::Earth));

	// Load .spritefont file and make it ready.
	m_font = std::make_unique<SpriteFont>(m_d3dDevice.Get(), resourceUpload,
		L"courier.spritefont",
		m_resourceDescriptors->GetCpuHandle(Descriptors::Courier),
		m_resourceDescriptors->GetGpuHandle(Descriptors::Courier));

	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_d3dDevice.Get());
	

	SpriteBatchPipelineStateDescription sprite_pd(rtState);

	// basic effect initialization
	m_gridEffect = std::make_unique<BasicEffect>(m_d3dDevice.Get(), EffectFlags::VertexColor, effect_pd);
	m_shapeEffect = std::make_unique<BasicEffect>(m_d3dDevice.Get(), EffectFlags::PerPixelLighting | EffectFlags::Texture, shape_pd);
	m_shapeEffect->SetLightEnabled(0, true);
	m_shapeEffect->SetLightDiffuseColor(0, Colors::White);
	m_shapeEffect->SetLightDirection(0, Vector3(-1.0f, -0.50f, 1.0f));
	m_shapeEffect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::Earth),
		m_states->AnisotropicWrap());

	// spritebatch init for text
	m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dDevice.Get(), resourceUpload, sprite_pd);

	// shape init
	//ShapeObject shape1;
	//ShapeObject shape2;
	//m_shapes.push_back(shape1);
	//m_shapes.push_back(shape2);

	m_shape = GeometricPrimitive::CreateSphere();
	//m_shape2 = GeometricPrimitive::CreateTorus();
	

	m_world = Matrix::Identity;

	auto uploadResourcesFinished = resourceUpload.End(m_commandQueue.Get()); // ResourceUploadEndHere

	uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        m_renderTargets[n].Reset();
        m_fenceValues[n] = m_fenceValues[m_backBufferIndex];
    }

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(c_swapBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = c_swapBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;
        DX::ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            swapChain.GetAddressOf()
            ));

        DX::ThrowIfFailed(swapChain.As(&m_swapChain));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        DX::ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_renderTargets[n]->SetName(name);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), n, m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
    // on this surface.
    CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
		4  // <---- Use 4x MSAA
        );
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = depthBufferFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &depthHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));

    m_depthStencil->SetName(L"Depth stencil");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthBufferFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS; // <---- use MSAA version

    m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // TODO: Initialize windows-size dependent objects here. //CreateResourcesHere
	// Set DirectX viewport
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f,
		static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight),
		D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	
	// msaa resource desription
	D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		backBufferFormat,
		backBufferWidth,
		backBufferHeight,
		1, // This render target view has only one texture.
		1, // Use a single mipmap level
		4  // <--- Use 4x MSAA 
	);
	msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE msaaOptimizedClearValue = {};
	msaaOptimizedClearValue.Format = backBufferFormat;
	memcpy(msaaOptimizedClearValue.Color, Colors::CornflowerBlue, sizeof(float) * 4);

	DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&depthHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&msaaRTDesc,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		&msaaOptimizedClearValue,
		IID_PPV_ARGS(m_offscreenRenderTarget.ReleaseAndGetAddressOf())
	));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
		m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		c_swapBufferCount, m_rtvDescriptorSize);
	m_d3dDevice->CreateRenderTargetView(m_offscreenRenderTarget.Get(), nullptr, rtvDescriptor);

	// set fullscreen rectangle
	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = backBufferWidth;
	m_fullscreenRect.bottom = backBufferHeight;

	m_spriteBatch->SetViewport(viewport);

	m_camera.UpdateViewMatrix();

	m_earthRotation = Matrix::CreateRotationX(0.f);
	// set effect matrices
	m_gridEffect->SetView(m_camera.GetView());
	m_gridEffect->SetProjection(m_camera.GetProj());

	
}

void Game::WaitForGpu() noexcept
{
    if (m_commandQueue && m_fence && m_fenceEvent.IsValid())
    {
        // Schedule a Signal command in the GPU queue.
        UINT64 fenceValue = m_fenceValues[m_backBufferIndex];
        if (SUCCEEDED(m_commandQueue->Signal(m_fence.Get(), fenceValue)))
        {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent.Get())))
            {
                WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                m_fenceValues[m_backBufferIndex]++;
            }
        }
    }
}

void Game::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_backBufferIndex];
    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the back buffer index.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_backBufferIndex])
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
        WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_backBufferIndex] = currentFenceValue + 1;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void Game::GetAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        DX::ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_featureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter)
    {
        if (FAILED(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
        {
            throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }
    }
#endif

    if (!adapter)
    {
        throw std::exception("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.Detach();
}

void Game::OnDeviceLost()
{
    // TODO: Perform Direct3D resource cleanup. // ondevicelosthere
	m_graphicsMemory.reset();
	m_font.reset();
	m_shapeEffect.reset();
	m_offscreenRenderTarget.Reset();
	m_resourceDescriptors.reset();
	m_spriteBatch.reset();
	m_gridEffect.reset();
	m_batch.reset();
	m_background.Reset();
	m_texture.Reset();
	m_states.reset();

	// reset all shapes
	/*
	for (auto&& itr : m_renderItems) {
		itr->Geo.reset();
	}
	*/

    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        m_commandAllocators[n].Reset();
        m_renderTargets[n].Reset();
    }

    m_depthStencil.Reset();
    m_fence.Reset();
    m_commandList.Reset();
    m_swapChain.Reset();
    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();
    m_commandQueue.Reset();
    m_d3dDevice.Reset();
    m_dxgiFactory.Reset();

    CreateDevice();
    CreateResources();
}

void Game::drawText(const char * asciiString, const Vector2 &pos)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring output = converter.from_bytes(asciiString);

	ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	//Vector2 origin = m_font->MeasureString(output.c_str()) / 2.f; // sets text origin to center
	Vector2 origin = { 0.0f, 0.0f }; // set text origin to upper left corner
	Vector2 fontPos = pos;

	m_font->DrawString(m_spriteBatch.get(), output.c_str(),
		fontPos, Colors::White, 0.f, origin);

}

void Game::drawGrid(Vector3 xaxis,
					Vector3 yaxis,
					Vector3 origin,
					XMFLOAT4 color,
					size_t divisions)
{

	for (size_t i = 0; i <= divisions; ++i)
	{
		float fPercent = float(i) / float(divisions);
		fPercent = (fPercent * 2.0f) - 1.0f;

		Vector3 scale = xaxis * fPercent + origin;

		VertexPositionColor v1(scale - yaxis, color);
		VertexPositionColor v2(scale + yaxis, color);
		m_batch->DrawLine(v1, v2);
	}

	for (size_t i = 0; i <= divisions; i++)
	{
		float fPercent = float(i) / float(divisions);
		fPercent = (fPercent * 2.0f) - 1.0f;

		Vector3 scale = yaxis * fPercent + origin;

		VertexPositionColor v1(scale - xaxis, color);
		VertexPositionColor v2(scale + xaxis, color);
		m_batch->DrawLine(v1, v2);
	}

	
}

void Game::BuildRenderItems()
{
	/*
	
	auto renderItem = std::make_unique<RenderItem>();
	renderItem->World = Matrix::Identity;
	
	//skullRitem->ObjCBIndex = 0;
	//skullRitem->Mat = mMaterials["tile0"].get();
	renderItem->Geo = std::make_unique<GeometricPrimitive>();
	renderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// Generate instance data.
	const int n = 5;


	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;

	float x = -0.5f*width;
	float y = -0.5f*height;
	float z = -0.5f*depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);
	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				int index = k * n*n + i * n + j;
				// Position instanced along a 3D grid.
				renderItem->World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					x + j * dx, y + i * dy, z + k * dz, 1.0f);

			}
		}
	}

	m_renderItems.push_back(std::move(renderItem));
	*/
}
