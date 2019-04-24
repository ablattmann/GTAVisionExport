#include "main.h"
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_4.h>
//#include <d3d11_3.h>
#include <wrl.h>
#include <ShlObj.h>
#include <system_error>
#include <string>
#include <filesystem>
#include <wincodec.h>
#include <cstdio>
#include <MinHook.h>
#include <cassert>
#include <chrono>
#include "export.h"
#include <d3d11shader.h>
#include <queue>
#include <d3dcompiler.h>
#include <vector>
#include <memory>
#include <Eigen/Core>
#include "scenario.h"
using Microsoft::WRL::ComPtr;
using namespace std::experimental::filesystem;
using namespace std::string_literals;
using std::chrono::milliseconds;
using std::chrono::time_point;
using std::chrono::system_clock;
using std::vector;
using Eigen::Matrix4f;
using Eigen::Vector3f;
using Eigen::Vector4f;
typedef void(*draw_indexed_hook_t)(ID3D11DeviceContext*, UINT, UINT, INT);
// ScriptHookV Functions
void scriptMain();
void presentCallback(void* chain);
void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
bool IsKeyDown(DWORD key);
bool IsKeyJustUp(DWORD key, bool exclusive = true);
void ResetKeyState(DWORD key);


//void draw_indexed_hook(ID3D11DeviceContext3* self, UINT IndexStart, UINT StartIndexLocation, INT BaseVertexLocation);
static time_point<system_clock> last_capture_color;
static time_point<system_clock> last_capture_depth;
static const char* logFilePath = "GTANativePlugin.log";
//--------
//offsets
//--------
const size_t drawIndexedOffset = 12;
const size_t clearDepthStencilViewOffset = 53;
//const size_t max_frames = 1000;
//-------------------------
//interesting D3D resources
//-------------------------
static ComPtr<ID3D11DepthStencilView> lastDsv;
static ComPtr<ID3D11RenderTargetView> lastRtv;
static ComPtr<ID3D11Buffer> lastConstants;

static bool saveNextFrame = false;
static bool hooked = false;

// added in order to be able to tell, if currently recording or not 
static bool recording = false;


//-------------------------
//global control variables
//-------------------------
static int draw_indexed_count = 0;

const std::string parameters_file = "param/parameters.txt";

// Keyboard variables
const int KEYS_SIZE = 255;
const int NOW_PERIOD = 100, MAX_DOWN = 5000; // ms

// FIXME make a read-function for the annotator in order not to require updating it every time
// the read-function will be called everytime , a new record is started
// FIXME add also a reset function, if required, that resets some states, here's a need of some more information
// FIXME the constructor should only initialize some required ressources, that will last for the entire time, it lives
static std::unique_ptr<DatasetAnnotator> annotator = std::make_unique<DatasetAnnotator>(parameters_file, false);


struct {
	DWORD time;
	BOOL isWithAlt;
	BOOL wasDownBefore;
	BOOL isUpNow;
} keyStates[KEYS_SIZE];

static void (_stdcall ID3D11DeviceContext::* origDrawInstanced)(UINT, UINT, INT) = nullptr;
static ComPtr<ID3D11DeviceContext> ctx;

int __stdcall DllMain(HMODULE hinstance, DWORD reason, LPVOID lpReserved)
{
	MH_STATUS res;
	auto f = fopen(logFilePath, "a");
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		res = MH_Initialize();
		if (res != MH_OK) fprintf(f, "Could not init Minihook\n");
		//scriptRegister(hinstance, scriptMain);
		presentCallbackRegister(presentCallback);
		keyboardHandlerRegister(OnKeyboardMessage);
		
		
		break;
	case DLL_PROCESS_DETACH:
		res = MH_Uninitialize();
		if (res != MH_OK) fprintf(f, "Could not deinit MiniHook\n");
		presentCallbackUnregister(presentCallback);
		keyboardHandlerUnregister(OnKeyboardMessage);
		//scriptUnregister(hinstance);

		break;
	}
	fclose(f);
	return TRUE;
}

// this has to be modified, in order to set up the keys states
void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	if (key < KEYS_SIZE)
	{
		keyStates[key].time = GetTickCount();
		keyStates[key].isWithAlt = isWithAlt;
		keyStates[key].wasDownBefore = wasDownBefore;
		keyStates[key].isUpNow = isUpNow;
	}

	if (key == 'R' && keyStates[key].isWithAlt && keyStates[key].wasDownBefore && keyStates[key].isUpNow && !recording) {
		annotator->drawText("Start recording!", static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 0.5f);
		recording = true;
	}else if (key == 'R' && keyStates[key].isWithAlt && keyStates[key].wasDownBefore && keyStates[key].isUpNow && recording) {
		annotator->drawText("Finish recording!", static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 0.5f);
		recording = false;
	}
}

bool IsKeyDown(DWORD key) {
	return (key < KEYS_SIZE) ? ((GetTickCount() < keyStates[key].time + MAX_DOWN) && !keyStates[key].isUpNow) : false;
}

bool IsKeyJustUp(DWORD key, bool exclusive)
{
	bool b = (key < KEYS_SIZE) ? (GetTickCount() < keyStates[key].time + NOW_PERIOD && keyStates[key].isUpNow) : false;
	if (b && exclusive)
		ResetKeyState(key);
	return b;
}

void ResetKeyState(DWORD key)
{
	if (key < KEYS_SIZE) {
		memset(&keyStates[key], 0, sizeof(keyStates[0]));
	}
}

inline void saveBuffersAndAnnotations(const int frame_nr) 
{	
	 //here we have to export all the data required, based on pushing a specific button (I would suggest 'L')
	//const auto output_as_wstr = annotator->getOutputPathW();
	const std::string depth_path = "depth_" + std::to_string(frame_nr) + ".raw";
	const std::string stenc_path = "stencil_" + std::to_string(frame_nr) + ".raw";
	const std::string col_path = "color_" + std::to_string(frame_nr) + ".raw";
	std::string out = annotator->getOutputPath();
	auto f = fopen(out.append(depth_path).c_str(), "w");
	void* buf;
	int size = export_get_depth_buffer(&buf);
	fwrite(buf, 1, size, f);
	fclose(f);
	f = fopen(out.append(stenc_path).c_str(), "w");
	size = export_get_stencil_buffer(&buf);
	fwrite(buf, 1, size, f);
	fclose(f);
	f = fopen(out.append(col_path).c_str(), "w");
	size = export_get_color_buffer(&buf);
	fwrite(buf, 1, size, f);
	fclose(f);
}

// This is a template function for hooks for arbitrary devices, i.e. arbitrary hook functions
template<int offset, typename T>
static void* orig;
template<int offset, typename T>
static void* targets;
template<int offset, typename T>
void hook_function(T* inst, void* hook, bool unhook = false)
{
	//__debugbreak();
	void** vtbl = *reinterpret_cast<void***>(inst);
	FILE* f = fopen("GTANativePlugin.log", "a");
	//fprintf(f, "Hooking %p at offset %d\n", inst, offset);
	MH_STATUS res = MH_OK;
	DWORD oldProt = 0;
	vtbl += offset;
	//VirtualProtect(vtbl, 8, PAGE_READWRITE, &oldProt);
	if (unhook)
	{
		res = MH_DisableHook(vtbl);
		if(res != MH_OK) fprintf(f, "error %d disabling hook at offset %d\n", res,  offset);
		orig<offset, T> = nullptr;
	}
	else {
		if(targets<offset, T> != nullptr && targets<offset, T> != *vtbl)
		{
			fprintf(f, "detected target change, someone else is screwing with our functions\n");
			res = MH_DisableHook(targets<offset, T>);
			if (res != MH_OK) fprintf(f, "errof %d disabling hook at offset %d\n", res, offset);
			res = MH_RemoveHook(targets<offset, T>);
			if (res != MH_OK) fprintf(f, "error %d removing hook at offset %d\n", res, offset);
			targets<offset, T> = nullptr;
			orig<offset, T> = nullptr;
		}
		if (orig<offset, T> == nullptr && targets<offset, T> != *vtbl) {
			// Here, the hook is created, using MinHookLibrary, 
			res = MH_CreateHook(*vtbl, hook, &(orig<offset, T>));
			if(res != MH_OK) fprintf(f, "error %d creating hook at offset %d\n",res, offset);
			
		}
		if (targets<offset, T> != *vtbl) {
			res = MH_EnableHook(*vtbl);
			if (res != MH_OK) fprintf(f, "error %d enabling hook at offset %d\n", res, offset);
			targets<offset, T> = *vtbl;
		}
		//*vtbl = reinterpret_cast<long long>(hook);
	}
	//VirtualProtect(vtbl, 8, oldProt, nullptr);
	//fprintf(f, "clear_hook: %p\n", hook);
	//fprintf(f, "clearFn: %p\n", (void*)(*(*reinterpret_cast<long long**>(inst) + 50)));
	fclose(f);
}

template<int offset, typename T>
void unhook_function(T* inst)
{
	hook_function<offset>(inst, nullptr, true);
}
void draw_hook_impl()
{
	FILE* f = fopen("DrawLog.log", "a");
	fprintf(f, "Draw Call\n");
	fclose(f);
}
void draw_indexed_hook(ID3D11DeviceContext* self, UINT indexCount, UINT startLoc, UINT baseLoc) {
	auto origMethod = reinterpret_cast<decltype(draw_indexed_hook)*>(orig<drawIndexedOffset, ID3D11DeviceContext>);
	HRESULT hr;
	ComPtr<ID3D11VertexShader> vs;
	self->VSGetShader(&vs, nullptr, nullptr);
	ComPtr<ID3D11Buffer> buf;
	ComPtr<ID3D11Device> dev;
	self->GetDevice(&dev);
	self->VSGetConstantBuffers(1, 1, &buf);
	if (buf != nullptr && draw_indexed_count == 1000) {
		lastConstants = buf;
		ExtractConstantBuffer(dev.Get(), self, buf.Get());
	}
	
	draw_indexed_count += 1;
	origMethod(self, indexCount, startLoc, baseLoc);
}
void clear_render_target_view_hook(ID3D11DeviceContext* self, ID3D11RenderTargetView* rtv, float color[4])
{
	auto origMethod = reinterpret_cast<void (*)(ID3D11DeviceContext*, ID3D11RenderTargetView*, float[4])>(orig<50, ID3D11DeviceContext>);
	
	ComPtr<ID3D11RenderTargetView> curRTV;
	self->OMGetRenderTargets(1, &curRTV, nullptr);
	// check if view can be obtained
	if (curRTV != nullptr)
	{
		D3D11_TEXTURE2D_DESC desc;
		ComPtr<ID3D11Resource> res;
		ComPtr<ID3D11Texture2D> tex;
		curRTV->GetResource(&res);
		HRESULT hr = S_OK;
		hr = res.As(&tex);
		if (hr != S_OK) return;
		tex->GetDesc(&desc);
		// check if the view is the one that's to be intended
		if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM && desc.Width > 600 && desc.Height > 600) {
			lastRtv = curRTV;
		}
	}
	origMethod(self, rtv, color);
}
void clear_depth_stencil_view_hook(ID3D11DeviceContext* self, ID3D11DepthStencilView* dsv, UINT8 flags, float depth, UINT8 stencil)
{
	auto origMethod = reinterpret_cast<decltype(&clear_depth_stencil_view_hook)>(orig<53, ID3D11DeviceContext>);
	ComPtr<ID3D11DepthStencilView> curDSV;
	self->OMGetRenderTargets(1, nullptr, &curDSV);
	ComPtr<ID3D11Device> dev;
	self->GetDevice(&dev);
	if (curDSV != nullptr) {
		D3D11_TEXTURE2D_DESC desc;
		ComPtr<ID3D11Resource> res;
		ComPtr<ID3D11Texture2D> tex;
		curDSV->GetResource(&res);
		HRESULT hr = S_OK;
		hr = res.As(&tex);
		if (hr != S_OK) return;
		tex->GetDesc(&desc);
		
		if (lastDsv == nullptr && desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS && desc.Width > 600 && desc.Height > 600) {
			lastDsv = curDSV;
			ExtractDepthBuffer(dev.Get(), self, res.Get());
			last_capture_depth = system_clock::now();
		}
	}
	origMethod(self, dsv, flags, depth, stencil);
	
	
}



void presentCallback(void* chain)
{	
	draw_indexed_count = 0;
	FILE* f = fopen(logFilePath, "a");
	HRESULT hr = S_OK;
	// hier wird das Interface, welches die Graphikoberflächen beinhaltet gecastet, so dass es MS konform ist
	auto swapChain = static_cast<IDXGISwapChain*>(chain);
	ComPtr<ID3D11Device> dev;
	ComPtr<ID3D11DeviceContext> ctx;
	// Hole das Device mithilfe des Interfaces
	hr = swapChain->GetDevice(__uuidof(ID3D11Device), &dev);
	if (hr != S_OK) throw std::system_error(hr, std::system_category());
	// Hole den Kontext, mithilfe dessen auf die einzelnen Surfaces zugegriffen werden kann
	dev->GetImmediateContext(&ctx);
	/*
	if (swapChain != hooked_chain)
	{
		if(hooked_chain != nullptr)
		{
			unhook_function<50>(ctx.Get());
			unhook_function<53>(ctx.Get());
		}
		ComPtr<ID3D10Multithread> multithread;
		hr = ctx.As(&multithread);
		if (hr != S_OK) throw std::system_error(hr, std::system_category());
		multithread->SetMultithreadProtected(true);
		hook_function<50>(ctx.Get(), &clear_render_target_view_hook);
		hook_function<53>(ctx.Get(), &clear_depth_stencil_view_hook);
		hooked_chain = swapChain;
	}*/
	ComPtr<ID3D11Multithread> multithread;
	// Check, dass alles OK ist, und dass der Pointer ein valides Interface darstellt
	hr = ctx.As(&multithread);
	if (hr != S_OK) throw std::system_error(hr, std::system_category());
	// guarantee thread safety
	multithread->SetMultithreadProtected(true);
	hook_function<drawIndexedOffset>(ctx.Get(), &draw_indexed_hook);
	//hook_function<50>(ctx.Get(), &clear_render_target_view_hook);
	hook_function<clearDepthStencilViewOffset>(ctx.Get(), &clear_depth_stencil_view_hook);
	if (f == nullptr) throw std::system_error(errno, std::system_category());
	
	ComPtr<ID3D11Resource> depthres;
	ComPtr<ID3D11Resource> colorres;
	ctx->OMGetRenderTargets(1, &lastRtv, nullptr);
	last_capture_color = system_clock::now();
	lastRtv->GetResource(&colorres);
	ExtractColorBuffer(dev.Get(), ctx.Get(), colorres.Get());
	if (recording) {
		// datasatAnnnotator extracts the files to respective repo
		const int frame_nr = annotator->update();

		// store buffers into the same repo
		saveBuffersAndAnnotations(frame_nr);

		if (frame_nr >= annotator->getMaxFrames()) {
			// stop recording
			recording = false;
		}
	}
	//lastDsv.Reset();
	lastDsv = nullptr;
	lastRtv = nullptr;
	fclose(f);
	
}