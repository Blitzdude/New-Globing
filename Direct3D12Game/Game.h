//
// Game.h
//

#pragma once

#include "StepTimer.h"

// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game
{
public:
    Game();
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
	// input and movement
	void OnKeyboardInput(DX::StepTimer const& timer);

	std::unique_ptr<DirectX::Keyboard>	m_keyboard;
	std::unique_ptr<DirectX::Mouse>		m_mouse;

    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void WaitForGpu() noexcept;
    void MoveToNextFrame();
    void GetAdapter(IDXGIAdapter1** ppAdapter);

    void OnDeviceLost();

	

    // Application state
    HWND                                                m_window;
    int                                                 m_outputWidth;
    int                                                 m_outputHeight;
	POINT												m_lastMousePosition;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                                   m_featureLevel;
    static const UINT                                   c_swapBufferCount = 2;
    UINT                                                m_backBufferIndex;
    UINT                                                m_rtvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4>               m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValues[c_swapBufferCount];
    Microsoft::WRL::Wrappers::Event                     m_fenceEvent;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

    // Game state
    DX::StepTimer                                       m_timer;

	// User variables *********************************//
	//***********************************************///
	// Graphics memory unique pointer
	std::unique_ptr<DirectX::GraphicsMemory>			m_graphicsMemory;
	std::unique_ptr<DirectX::DescriptorHeap>			m_resourceDescriptors;
	std::unique_ptr<DirectX::SpriteFont> m_font;

	Microsoft::WRL::ComPtr<ID3D12Resource>				m_background;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_offscreenRenderTarget;

	// Font attributes
	std::unique_ptr<DirectX::SpriteBatch>				m_spriteBatch;
	DirectX::SimpleMath::Vector2						m_origin;


	// Matrices
	DirectX::SimpleMath::Matrix							m_rotation;
	DirectX::SimpleMath::Matrix							m_world;


	// Camera
	Camera												m_camera;

	std::unique_ptr<DirectX::GeometricPrimitive>		m_shape;
	//std::unique_ptr<DirectX::GeometricPrimitive>		m_shape2;


	// effect rendering
	std::unique_ptr<DirectX::BasicEffect> m_gridEffect;
	std::unique_ptr<DirectX::BasicEffect> m_shapeEffect;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;

	// Rect descriptors
	RECT m_fullscreenRect;
	enum Descriptors
	{
		MyFont,
		Background,
		Count
	};

	// ************************************************//
	// User Methods ********************************//
	void drawText(const char* asciiString, const DirectX::SimpleMath::Vector2 &pos);
	void drawGrid(	DirectX::SimpleMath::Vector3 xaxis,
					DirectX::SimpleMath::Vector3 yaxis,
					DirectX::SimpleMath::Vector3 origin,
					DirectX::XMFLOAT4 color,
					size_t divisions);

	void BuildRenderItems();

	// *******************************************//
};
