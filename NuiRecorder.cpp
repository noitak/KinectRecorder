#include "StdAfx.h"
#include "NuiRecorder.h"

// NuiFrameTextureImpl
NuiFrameTextureImpl::NuiFrameTextureImpl() {
	_refCount = 0;
	_bufferLen = 0;
	_pitch = 0;
	_lockedRect.Pitch = 0;
	_lockedRect.size = 0;
	_lockedRect.pBits = NULL;
	_desc.Width = 0;
	_desc.Height = 0;
}
NuiFrameTextureImpl::NuiFrameTextureImpl(INuiFrameTexture& other) {
	_refCount = 0;
	_lockedRect.pBits = NULL;
	_bufferLen = other.BufferLen();
	_pitch = other.Pitch();
	other.LockRect(0, &_lockedRect, NULL, 0);
	other.GetLevelDesc(0, &_desc);
}
NuiFrameTextureImpl::~NuiFrameTextureImpl() {
	if (_lockedRect.pBits) { delete[] _lockedRect.pBits; }
}


// NuiImageFrameWrapper
NuiImageFrameWrapper::NuiImageFrameWrapper() {
	_imageFrame.pFrameTexture = NULL;
}
NuiImageFrameWrapper::NuiImageFrameWrapper(const NUI_IMAGE_FRAME& imageFrame) {
	_imageFrame.liTimeStamp = imageFrame.liTimeStamp;
	_imageFrame.dwFrameNumber = imageFrame.dwFrameNumber;
	_imageFrame.eImageType = imageFrame.eImageType;
	_imageFrame.eResolution = imageFrame.eResolution;
	_imageFrame.dwFrameFlags = imageFrame.dwFrameFlags;
	_imageFrame.pFrameTexture = new NuiFrameTextureImpl(*imageFrame.pFrameTexture);
	_imageFrame.ViewArea = imageFrame.ViewArea;
}
NuiImageFrameWrapper::~NuiImageFrameWrapper() {
	delete _imageFrame.pFrameTexture;
}
void NuiImageFrameWrapper::GetImageFrame(NUI_IMAGE_FRAME& imageFrame) {
	imageFrame.liTimeStamp = _imageFrame.liTimeStamp;
	imageFrame.dwFrameNumber = _imageFrame.dwFrameNumber;
	imageFrame.eImageType = _imageFrame.eImageType;
	imageFrame.eResolution = _imageFrame.eResolution;
	imageFrame.dwFrameFlags = _imageFrame.dwFrameFlags;
	imageFrame.pFrameTexture = new NuiFrameTextureImpl(*_imageFrame.pFrameTexture);
	imageFrame.ViewArea = _imageFrame.ViewArea;
}

NuiRecorder::NuiRecorder(void) :_stream(NULL), _archive(NULL) {
}

NuiRecorder::~NuiRecorder(void) {
	Close();
}

bool NuiRecorder::Open(const char* filename)
{
	Close();
	try {
		_stream = new std::ofstream(filename);
		_archive = new boost::archive::binary_oarchive(*_stream);
	} catch (std::exception& e) {
		return false;
	}
	return true;
}

void NuiRecorder::Close() {
	if (_archive) {
		delete _archive;
		_archive = NULL;
	}
	if (_stream) {
		_stream->close();
		delete _stream;
		_stream = NULL;
	}
}

bool NuiRecorder::Record(const NUI_IMAGE_FRAME& imageFrame) {
	if (!IsOpened()) { return false; }
	try {
		NuiImageFrameWrapper ifw(imageFrame);
		*_archive << ifw;
	} catch (std::exception& e) {
		return false;
	}
	return true;
}

NuiPlayer::NuiPlayer(void) :_stream(NULL), _archive(NULL) {
}
NuiPlayer::~NuiPlayer(void) {
	Close();
}
bool NuiPlayer::Open(const char* filename) {
	Close();
	try {
		_stream = new std::ifstream(filename);
		_archive = new boost::archive::binary_iarchive(*_stream);
	} catch (std::exception& e) {
		return false;
	}
	return true;
}
void NuiPlayer::Close() {
	if (_archive) {
		delete _archive;
		_archive = NULL;
	}
	if (_stream) {
		_stream->close();
		delete _stream;
		_stream = NULL;
	}
}
bool NuiPlayer::Record(NUI_IMAGE_FRAME& imageFrame) {
	if (!IsOpened()) { return false; }
	try {
		NuiImageFrameWrapper ifw;
		*_archive >> ifw;
		ifw.GetImageFrame(imageFrame);
	} catch (std::exception& e) {
		return false;
	}
	return true;
}
