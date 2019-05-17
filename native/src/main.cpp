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
#include <wingdi.h>
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
#include "keyboard.h"
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
//void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
//bool IsKeyDown(DWORD key);
//bool IsKeyJustUp(DWORD key, bool exclusive = true);
//void ResetKeyState(DWORD key);


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
static bool load_new = false;
static bool reset = false;


//-------------------------
//global control variables
//-------------------------
static int draw_indexed_count = 0;

const std::string parameters_file = "param\\parameters.txt";
static std::unique_ptr<DatasetAnnotator> annotator=std::make_unique<DatasetAnnotator>(parameters_file,false);

// FIXME make a read-function for the annotator in order not to require updating it every time
// the read-function will be called everytime , a new record is started
// FIXME add also a reset function, if required, that resets some states, here's a need of some more information
// FIXME the constructor should only initialize some required ressources, that will last for the entire time, it lives
//static std::unique_ptr<DatasetAnnotator> annotator = std::make_unique<DatasetAnnotator>(parameters_file, false);


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
		presentCallbackRegister(presentCallback);
		keyboardHandlerRegister(OnKeyboardMessage);
		scriptRegister(hinstance, scriptMain);
		
		break;
	case DLL_PROCESS_DETACH:
		res = MH_Uninitialize();
		if (res != MH_OK) fprintf(f, "Could not deinit MiniHook\n");
		scriptUnregister(hinstance);
		presentCallbackUnregister(presentCallback);
		keyboardHandlerUnregister(OnKeyboardMessage);
		

		break;
	}
	fclose(f);
	return TRUE;
}

inline int StringToWString(std::wstring &ws, const std::string &s)
{
	std::wstring wsTmp(s.begin(), s.end());
	ws = wsTmp;
	return 0;
}

inline void exportStencilToBitmap(void** buf, std::size_t size, const std::string& output_path) {
	const auto window_width = annotator->getWindowWidth();
	const auto window_height = annotator->getWindowHeight();
	

	std::wstring ws;
	StringToWString(ws, output_path);

	FILE* log = fopen("GTANativePlugin.log", "a");

	if (static_cast<std::size_t>(window_width * window_height) != size) {

		

		int res_x, res_y;
		GRAPHICS::_GET_SCREEN_ACTIVE_RESOLUTION(&res_x,&res_y);

		if (size == res_y * res_x) {
			fprintf(log, "WARNING: window height and width are not equal to number of elements in semantic mask due to Screen Resolution! Saving downsampled mask.\n");
			auto helper = static_cast<BYTE *>(*buf);
			Gdiplus::Bitmap image(res_x, res_y, res_x, PixelFormat8bppIndexed, helper);
			fprintf(log, "Status after image instantiating is %d\n", image.GetLastStatus());

			const auto bmp_id = annotator->getCLSID();
			image.Save(ws.c_str(), &bmp_id, NULL);

			fprintf(log, "Status after image saving is %d\n", image.GetLastStatus());
			
		}
		else {
			fprintf(log, "WARNING: window height and width are not equal to number of elements in semantic mask due to Screen Resolution! Not saving semantic mask.\n");
		}
		fclose(log);
		return;
	}

	fprintf(log, "Before instantiating helper.\n");
	auto helper = static_cast<BYTE *>(*buf);
	fprintf(log, "After instantiating helper.\n");
	//for (std::size_t i = 0; i < size ; i++) {
	//	// use mask to only retain the first 4 Bits
	//	helper[i] = helper[i] & 0x0F;
	//}
	//fprintf(log, "After mask loop.\n");
	/*Gdiplus::PixelFormat px_format = Gdiplus::PixekFormat.PixelFormat8bppIndexed;*/
	//BYTE* ordered = new BYTE[window_height * window_width];
	//for (int x = 0; x < window_width; x++) {
	//	for (int y = 0; y < window_height; y++) {
	//		// fixme add bpp here and try again, see copyTexToVector for Reference 
	//		ordered[x + y * window_width] = helper[x * window_height + y ];

	//	}
	//}
	fprintf(log, "After mask loop.\n");
	
	//auto img_data = static_cast<BYTE *>(*buf);
	// stride is window width as we want to constructn image with one byte depth
	Gdiplus::Bitmap image(window_width, window_height, window_width, PixelFormat8bppIndexed , helper);
	fprintf(log, "Status after image instantiating is %d\n", image.GetLastStatus());

	const auto bmp_id = annotator->getCLSID();
	image.Save(ws.c_str(), &bmp_id , NULL);
	
	fprintf(log,"Status after image saving is %d\n", image.GetLastStatus());
	fclose(log);
	//delete[] ordered;
}

inline void exportPNG(void** buf, std::size_t size, const std::string& output_path) {
	
	const auto window_width = annotator->getWindowWidth();
	const auto window_height = annotator->getWindowHeight();

	std::wstring ws;
	StringToWString(ws, output_path);

	FILE* log = fopen("GTANativePlugin.log", "a");
	
	// multiply with 4 because of RGBA representation of the images
	if (size != window_width * window_height*4) {
		int res_x, res_y;
		GRAPHICS::GET_SCREEN_RESOLUTION(&res_x, &res_y);

		if (size == res_y * res_x * 4) {
			fprintf(log, "WARNING: window height and width are not equal to number of elements in semantic mask due to Screen Resolution! Saving downsampled image.\n");
			auto helper = static_cast<BYTE *>(*buf);
			Gdiplus::Bitmap image(res_x, res_y, res_x, PixelFormat32bppARGB, helper);
			fprintf(log, "Status after image instantiating is %d\n", image.GetLastStatus());

			const auto png_id = annotator->getCLSIDPNG();
			image.Save(ws.c_str(), &png_id, NULL);

			fprintf(log, "Status after image saving is %d\n", image.GetLastStatus());

		}
		else {
			fprintf(log, "WARNING: window height and width are not equal to number of elements in semantic mask due to Screen Resolution! Not saving image.\n");
		}
		fclose(log);
		return;
	}

	fprintf(log,"Saving image.\n");
	auto helper = static_cast<BYTE *>(*buf);
	Gdiplus::Bitmap image(window_width, window_height, window_width, PixelFormat32bppARGB, helper);
	fprintf(log, "Status after image instantiating is %d\n", image.GetLastStatus());

	const auto png_id = annotator->getCLSIDPNG();
	image.Save(ws.c_str(), &png_id, NULL);

	fprintf(log, "Status after image saving is %d\n", image.GetLastStatus());
}

inline void saveBuffersAndAnnotations(const int frame_nr) 
{	
	 //here we have to export all the data required, based on pushing a specific button (I would suggest 'L')
	//const std::string depth_path = "depth_" + std::to_string(frame_nr) + ".raw";
	//const std::string stenc_path = "stencil_" + std::to_string(frame_nr) + ".raw";
	const std::string col_path = "frame_" + std::to_string(frame_nr) + ".raw";
	const std::string sem_string = "sem_map_" + std::to_string(frame_nr) + ".bmp";
	// logging, only at first frame,  in order to avoid spamming the programm
	//if (frame_nr == 0) {
	//	FILE* log = fopen("GTANativePlugin.log", "a");
	//	fprintf(log, "Saving files; Depth path is %s\n", depth_path);
	//	fprintf(log, "Saving files; Stencil path is %s\n", stenc_path);
	//	fprintf(log, "Saving files; Color path is %s\n", col_path);
	//	fclose(log);
	//}
	// save Files

	std::string out = annotator->getOutputPath();
	// auto f = fopen((out + "\\" + depth_path).c_str(), "w");
	void* stenc_buf;
	/*int size = export_get_depth_buffer(&buf);
	fwrite(buf, 1, size, f);
	fclose(f)*/;
	/*f = fopen((out + "\\" + stenc_path).c_str(), "w");*/
	int size = export_get_stencil_buffer(&stenc_buf);
	exportStencilToBitmap(&stenc_buf,size,out + "\\" + sem_string);
	/*fwrite(buf, 1, size, f);
	fclose(f);*/
	void* buf;
	auto f = fopen((out + "\\" + col_path).c_str(), "w");
	size = export_get_color_buffer(&buf);
	// FIXME debug this function in order not to be dependend of postprocessing
	//exportPNG(&buf,size,out + "\\" + col_path);
	fwrite(buf, 1, size, f);
	fclose(f);
	buf = nullptr;
	stenc_buf = nullptr;
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
		/*FILE* f = fopen("GTANativePlugin.log", "a");
		fprintf(f, "Render-Target-Hook: desc.Width is %d and desc.Height is %d\n", desc.Width, desc.Height);
		fclose(f);*/
		if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM && desc.Width >= 1000 && desc.Height >= 1000) {
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
		/*FILE* f = fopen("GTANativePlugin.log", "a");
		fprintf(f, "Stencil-Hook: desc.Width is %d and desc.Height is %d\n", desc.Width, desc.Height);
		fclose(f);*/
		if (lastDsv == nullptr && desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS && desc.Width >= 1000 && desc.Height >= 1000) {
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

		// WAIT

		// store buffers into the same repo
		if(frame_nr>=0) saveBuffersAndAnnotations(frame_nr);
		

		if (frame_nr >= annotator->getMaxFrames()) {
			// stop recording
			recording = false;
			if (annotator->recordAllSeqs()) {
				reset=true;
			}
			// fixme add info message here
		}
	}
	//lastDsv.Reset();
	lastDsv = nullptr;
	lastRtv = nullptr;
	fclose(f);
}

void reactionOnKeyboard() {

	if (annotator->recordAllSeqs()) {
		while (true) {
			//annotator->drawText("ScriptMain called!!!");
			if (IsKeyJustUp(VK_F3) || load_new){
				//annotator->drawText("Loading Scenario!");
				load_new = false;
				annotator->loadScenario();
				Sleep(100);
				recording = true;
				//annotator->drawText("Start recording!");
			}
			else if (reset) {
				recording = false;
				reset = false;
				annotator->drawText("Finish recording!");
				annotator->resetStates();
				WAIT(5000);
				load_new = true;
				Sleep(100);
			}
			WAIT(0);
		}
	}
	else {
		while (true) {
			//annotator->drawText("ScriptMain called!!!");
			if (IsKeyJustUp(VK_F3) && !recording) {
				annotator->drawText("Loading Scenario!");
				annotator->loadScenario();
				Sleep(100);
				recording = true;
				//annotator->drawText("Start recording!");
			}
			else if ((IsKeyJustUp(VK_F2) || IsKeyJustUp(VK_ESCAPE)) && recording) {
				recording = false;
				annotator->drawText("Finish recording!");
				annotator->resetStates();
				Sleep(100);
			}
			WAIT(0);
		}
	}
	
	
}

void scriptMain() {
	
	srand(GetTickCount());
	reactionOnKeyboard();
	
}