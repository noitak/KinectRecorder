#pragma once

#include <fstream>
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <NuiApi.h>

/** Dummy implementation for INuiFrameTexture interface
 */
class NuiFrameTextureImpl : public INuiFrameTexture
{
private:
	ULONG _refCount;
	int _bufferLen;
	int _pitch;
	NUI_LOCKED_RECT _lockedRect;
	NUI_SURFACE_DESC _desc;

public:
	NuiFrameTextureImpl();
	NuiFrameTextureImpl(INuiFrameTexture& other);
	~NuiFrameTextureImpl();

	virtual ULONG STDMETHODCALLTYPE AddRef() {
		return ++_refCount;
	}
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
		*ppvObject = NULL;
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppvObject = (IUnknown*)(INuiFrameTexture*)this;
		} else if (IsEqualIID(riid, IID_INuiFrameTexture)) {
			*ppvObject = (INuiFrameTexture*)this;
		}
		if (*ppvObject) {
			AddRef();
			return S_OK;
		}
		return (HRESULT)E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE Release() {
		ULONG count = --_refCount;
		if (_refCount == 0) { delete this; }
		return count;
	}
	virtual int STDMETHODCALLTYPE BufferLen( void) { 
		return _bufferLen; 
	}
	virtual int STDMETHODCALLTYPE Pitch( void) { 
		return _pitch; 
	}
	virtual HRESULT STDMETHODCALLTYPE LockRect( 
		UINT Level,
		/* [ref] */ NUI_LOCKED_RECT *pLockedRect,
		/* [unique] */ RECT *pRect,
		DWORD Flags) {
			copyLockedRect(_lockedRect, *pLockedRect);
			return (HRESULT)0;
	}
	virtual HRESULT STDMETHODCALLTYPE GetLevelDesc( 
		UINT Level,
		NUI_SURFACE_DESC *pDesc) {
			pDesc->Width = _desc.Width;
			pDesc->Height = _desc.Height;
			return (HRESULT)0;
	}
	virtual HRESULT STDMETHODCALLTYPE UnlockRect( 
		/* [in] */ UINT Level) {
			return (HRESULT)0;
	}

	void SetBufferLen(int bufferLen) { _bufferLen = bufferLen; }
	void SetPitch(int pitch) { _pitch = pitch; }
	void SetLockedRect(int pitch, int size, byte* pByte) {
		_lockedRect.Pitch = pitch;
		_lockedRect.size = size;
		if (_lockedRect.pBits) { delete[] _lockedRect.pBits; }
		_lockedRect.pBits = new byte[size];
		memcpy_s(_lockedRect.pBits, sizeof(_lockedRect.pBits), pByte, size * sizeof(byte));
	}
	void SetLockedRect(const NUI_LOCKED_RECT& rect) {
		SetLockedRect(rect.Pitch, rect.size, rect.pBits);
	}
	void SetSurfaceDesc(UINT width, UINT height) {
		_desc.Width = width;
		_desc.Height = height;
	}
	void SetSurfaceDesc(const NUI_SURFACE_DESC& desc) {
		SetSurfaceDesc(desc.Width, desc.Height);
	}
	void copyLockedRect(const NUI_LOCKED_RECT& src, NUI_LOCKED_RECT& dst) {
		dst.Pitch = src.Pitch;
		dst.size = src.size;
		if (dst.pBits) { delete[] dst.pBits; }
		dst.pBits = new byte[src.size];
		memcpy_s(dst.pBits, sizeof(dst.pBits), src.pBits, src.size * sizeof(byte));
	}
};

/** Wrapper class for NUI_IMAGE_FRAME
 */
class NuiImageFrameWrapper 
{
private:
	NUI_IMAGE_FRAME _imageFrame;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & _imageFrame.liTimeStamp.LowPart;
		ar & _imageFrame.liTimeStamp.HighPart;
		ar & _imageFrame.dwFrameNumber;
		ar & _imageFrame.eImageType;
		ar & _imageFrame.eResolution;
		if (_imageFrame.pFrameTexture) {
			int bufferLen = _imageFrame.pFrameTexture->BufferLen();
			ar & bufferLen;
			int pitch = _imageFrame.pFrameTexture->Pitch();
			ar & pitch;
			NUI_LOCKED_RECT rect;
			rect.pBits = NULL;
			_imageFrame.pFrameTexture->LockRect(0, &rect, NULL, 0);
			ar & rect.Pitch;
			ar & rect.size;
			for (int i = 0; i < rect.size; i++) {
				ar & rect.pBits[i];
			}
			NUI_SURFACE_DESC desc;
			_imageFrame.pFrameTexture->GetLevelDesc(0, &desc);
			ar & desc.Width;
			ar & desc.Height;
		} else {
			NuiFrameTextureImpl* fti = new NuiFrameTextureImpl;
			int bufferLen;
			ar & bufferLen;
			fti->SetBufferLen(bufferLen);
			int pitch;
			ar & pitch;
			fti->SetPitch(pitch);
			NUI_LOCKED_RECT rect;
			ar & rect.Pitch;
			ar & rect.size;
			rect.pBits = new byte[rect.size];
			for (int i = 0; i < rect.size; i++) {
				ar & rect.pBits[i];
			}
			fti->SetLockedRect(rect);
			NUI_SURFACE_DESC desc;
			ar & desc.Width;
			ar & desc.Height;
			fti->SetSurfaceDesc(desc);
			_imageFrame.pFrameTexture = fti;
		}
		ar & _imageFrame.dwFrameFlags;
		ar & _imageFrame.ViewArea.eDigitalZoom;
		ar & _imageFrame.ViewArea.lCenterX;
		ar & _imageFrame.ViewArea.lCenterY;
	}
public:
	NuiImageFrameWrapper();
	NuiImageFrameWrapper(const NUI_IMAGE_FRAME& imageFrame);
	~NuiImageFrameWrapper();
	void GetImageFrame(NUI_IMAGE_FRAME& imageFrame);
};

/** Recorder for Microsoft Kinect SDK
 */
class NuiRecorder
{
protected:
	std::ofstream* _stream;
	boost::archive::binary_oarchive* _archive;

public:
	NuiRecorder(void);
	~NuiRecorder(void);
	bool Open(LPCTSTR filename);
	void Close();
	bool Record(const NUI_IMAGE_FRAME& imageFrame);
	bool IsOpened() { return (_stream && _archive); }
};

/** Player for Microsoft Kinect SDK
 */
class NuiPlayper
{
protected:
	std::ifstream* _stream;
	boost::archive::binary_iarchive* _archive;

public:
	NuiPlayper(void);
	~NuiPlayper(void);
	bool Open(LPCTSTR filename);
	void Close();
	bool Record(NUI_IMAGE_FRAME& imageFrame);
	bool IsOpened() { return (_stream && _archive); }
};
