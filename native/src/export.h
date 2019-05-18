#ifndef GTA_VISION_NATIVE_EXPORT_H
#define GTA_VISION_NATIVE_EXPORT_H
#include <d3d11.h>
void ExtractDepthBuffer(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Resource* tex);
void ExtractColorBuffer(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Resource* tex);
//void ExtractConstantBuffer(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Buffer* buf);
void CopyIfRequested();

extern "C" {
	__declspec(dllexport) int export_get_depth_buffer(void** buf);
	__declspec(dllexport) int export_get_color_buffer(void** buf);
	__declspec(dllexport) int export_get_stencil_buffer(void** buf);
}
#endif