//
//	SpoutFunctions.cpp
//
//	A Spout functions class specifically for SpoutLibrary.
//  Includes both sender and receiver functions
//	but a separate instance of SpoutLibrary is required
//	for individual senders or receivers.
//
/*
		Copyright (c) 2016-2019, Lynn Jarvis. All rights reserved.

		Redistribution and use in source and binary forms, with or without modification, 
		are permitted provided that the following conditions are met:

		1. Redistributions of source code must retain the above copyright notice, 
		   this list of conditions and the following disclaimer.

		2. Redistributions in binary form must reproduce the above copyright notice, 
		   this list of conditions and the following disclaimer in the documentation 
		   and/or other materials provided with the distribution.

		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"	AND ANY 
		EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
		OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE	ARE DISCLAIMED. 
		IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
		INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
		PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
		INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
		LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
		OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "SpoutFunctions.h"

/*
// Log level definitions
enum SpoutLogLevel {
	SPOUT_LOG_SILENT,
	SPOUT_LOG_VERBOSE,
	SPOUT_LOG_NOTICE,
	SPOUT_LOG_WARNING,
	SPOUT_LOG_ERROR,
	SPOUT_LOG_FATAL
};
*/

SpoutFunctions::SpoutFunctions() {

	bIsSending = false;
	m_SenderNameSetup[0] = 0;
	m_SenderName[0] = 0;
	m_TextureID = 0;
	m_TextureTarget = 0;
	m_bInvert = false;
	m_bUpdate = false;
	m_bUseActive = false;
	m_Width = 0;
	m_Height = 0;

};

SpoutFunctions::~SpoutFunctions() {
	if(bIsSending)
		CloseSender();
	else
		CloseReceiver();
};

// ---------------------------------------------------------------
// 2.007


//
// Sender
//

//---------------------------------------------------------
bool SpoutFunctions::SetupSender(const char* SenderName,
	unsigned int width, unsigned int height, bool bInvert, DWORD dwFormat)
{
	strcpy_s(m_SenderName, 256, SenderName);
	m_bInvert = bInvert;
	m_Width = width;
	m_Height = height;
	return CreateSender(m_SenderName, m_Width, m_Height, dwFormat);
}

//---------------------------------------------------------
void SpoutFunctions::Update(unsigned int width, unsigned int height)
{
	if (width != m_Width || height != m_Height) {
		UpdateSender(m_SenderName, width, height);
		m_Width = width;
		m_Height = height;
	}
}

//---------------------------------------------------------
bool SpoutFunctions::IsInitialized()
{
	return spout.IsSpoutInitialized();
}

//---------------------------------------------------------
void SpoutFunctions::CloseSender()
{
	if (IsInitialized())
		ReleaseSender();
	m_SenderName[0] = 0;
	m_bInvert = true;
	m_Width = 0;
	m_Height = 0;
}

//---------------------------------------------------------
bool SpoutFunctions::SendTextureData(GLuint TextureID, GLuint TextureTarget, GLuint HostFbo)
{
	if (IsInitialized())
		return SendTexture(TextureID, TextureTarget, m_Width, m_Height, m_bInvert, HostFbo);
	else
		return false;
}

//---------------------------------------------------------
bool SpoutFunctions::SendFboData(GLuint FboID)
{
	if (IsInitialized())
		return SendFboTexture(FboID, m_Width, m_Height, m_bInvert);
	else
		return false;
}

//---------------------------------------------------------
bool SpoutFunctions::SendImageData(const unsigned char* pixels, GLenum glFormat, GLuint HostFbo)
{
	if (IsInitialized())
		return SendImage(pixels, m_Width, m_Height, glFormat, m_bInvert, HostFbo);
	else
		return false;
}

//---------------------------------------------------------
unsigned int SpoutFunctions::GetWidth()
{
	return m_Width;
}

//---------------------------------------------------------
unsigned int SpoutFunctions::GetHeight()
{
	return m_Height;
}

//---------------------------------------------------------
long SpoutFunctions::GetFrame()
{
	return (spout.interop.frame.GetSenderFrame());
}

//---------------------------------------------------------
double SpoutFunctions::GetFps()
{
	return (spout.interop.frame.GetSenderFps());
}

//---------------------------------------------------------
void SpoutFunctions::HoldFps(int fps)
{
	return (spout.interop.frame.HoldFps(fps));
}


//
// Receiver
//

//---------------------------------------------------------
void SpoutFunctions::SetupReceiver(unsigned int width, unsigned int height, bool bInvert)
{
	// CreateReceiver will use the active sender
	// unless the user has specified a sender to 
	// connect to usint SetReceiverName
	m_SenderNameSetup[0] = 0;
	m_SenderName[0] = 0;
	m_bUseActive = true;

	// Record details for subsequent functions
	m_Width = width;
	m_Height = height;
	m_bInvert = bInvert;
	m_bUpdate = false;

}

//---------------------------------------------------------
void SpoutFunctions::SetReceiverName(const char * SenderName)
{
	if (SenderName && SenderName[0]) {
		strcpy_s(m_SenderNameSetup, 256, SenderName);
		strcpy_s(m_SenderName, 256, SenderName);
		m_bUseActive = false; // the user has specified a sender to connect to
	}
}

//---------------------------------------------------------
bool SpoutFunctions::IsUpdated()
{
	// Return whether the application texture needs updating.
	// The application must update the receiving texture before
	// the next call to ReceiveTexture when the update flag is reset.
	return m_bUpdate;
}

//---------------------------------------------------------
bool SpoutFunctions::IsConnected()
{
	return spout.IsSpoutInitialized();
}

//---------------------------------------------------------
void SpoutFunctions::CloseReceiver()
{
	ReleaseReceiver();
	// Restore the sender name that the user specified in SetupReceiver
	strcpy_s(m_SenderName, 256, m_SenderNameSetup);
	m_Width = 0;
	m_Height = 0;
}

//---------------------------------------------------------
void SpoutFunctions::SelectSender()
{
	spout.SelectSenderPanel();
}

//---------------------------------------------------------
bool SpoutFunctions::ReceiveTextureData(GLuint TextureID, GLuint TextureTarget, GLuint HostFbo)
{
	m_bUpdate = false;

	// Initialization is recorded in the spout class for sender or receiver
	if (!IsConnected()) {
		if (CreateReceiver(m_SenderName, m_Width, m_Height, m_bUseActive)) {
			// Signal the application to update the receiving texture size
			// Retrieved with a call to the Updated function
			m_bUpdate = true;
			return true;
		}
	}
	else {
		// Save sender name and dimensions to test for change
		char name[256];
		strcpy_s(name, 256, m_SenderName);
		unsigned int width = m_Width;
		unsigned int height = m_Height;
		// Receive a shared texture but don't read it into the user texture yet
		if (ReceiveTexture(name, width, height)) {
			// Test for sender name or size change
			if (width != m_Width
				|| height != m_Height
				|| strcmp(name, m_SenderName) != 0) {
				// Update name
				strcpy_s(m_SenderName, 256, name);
				// Update class dimensions
				m_Width = width;
				m_Height = height;
				// Signal the application to update the receiving texture
				m_bUpdate = true;
				return true;
			}
			else {
				// Read the shared texture to the user texture
				return spout.interop.ReadTexture(m_SenderName, TextureID, TextureTarget, m_Width, m_Height, false, HostFbo);
			}
		}
		else {
			// receiving failed
			CloseReceiver();
			return false;
		}
	}

	// No connection
	return false;

}

//---------------------------------------------------------
bool SpoutFunctions::ReceiveImageData(unsigned char *pixels, GLenum glFormat, GLuint HostFbo)
{
	m_bUpdate = false;

	if (!IsConnected()) {
		if (CreateReceiver(m_SenderName, m_Width, m_Height, m_bUseActive)) {
			m_bUpdate = true;
			return true;
		}
	}
	else {
		char sendername[256];
		strcpy_s(sendername, 256, m_SenderName);
		unsigned int width = m_Width;
		unsigned int height = m_Height;
		// Receive a shared image but don't read it into the user pixels yet
		if (ReceiveImage(sendername, width, height, NULL)) {
			// Test for sender name or size change
			if (width != m_Width
				|| height != m_Height
				|| strcmp(m_SenderName, sendername) != 0) {
				// Update the connected sender name
				strcpy_s(m_SenderName, 256, sendername);
				// Update class dimensions
				m_Width = width;
				m_Height = height;
				// Signal the application to update the receiving pixels
				m_bUpdate = true;
				return true;
			}
			else {
				// Read the shared texture or memory directly into the pixel buffer
				// Functions handle the formats supported
				return(spout.interop.ReadTexturePixels(m_SenderName, 
						pixels, width, height, glFormat, m_bInvert, HostFbo));
			}
		}
		else {
			// receiving failed
			CloseReceiver();
			return false;
		}
	}

	// No connection
	return false;

}

//---------------------------------------------------------
const char * SpoutFunctions::GetSenderName()
{
	return m_SenderName;
}

//---------------------------------------------------------
unsigned int SpoutFunctions::GetSenderWidth()
{
	return m_Width;
}

//---------------------------------------------------------
unsigned int SpoutFunctions::GetSenderHeight()
{
	return m_Height;
}

//---------------------------------------------------------
long SpoutFunctions::GetSenderFrame()
{
	return (spout.interop.frame.GetSenderFrame());
}

//---------------------------------------------------------
double SpoutFunctions::GetSenderFps()
{
	return (spout.interop.frame.GetSenderFps());
}

//---------------------------------------------------------
bool SpoutFunctions::IsFrameNew()
{
	return (spout.interop.frame.IsFrameNew());
}


// Common

//---------------------------------------------------------
void SpoutFunctions::DisableFrameCount()
{
	spout.interop.frame.DisableFrameCount();
}

//---------------------------------------------------------
bool SpoutFunctions::IsFrameCountEnabled()
{
	return spout.interop.frame.IsFrameCountEnabled();
}

//
// 2.006 and earlier
//

//---------------------------------------------------------
bool SpoutFunctions::CreateSender(const char* name, unsigned int width, unsigned int height, DWORD dwFormat)
{
	bIsSending = true;
	return spout.CreateSender(name, width, height, dwFormat);
}

//---------------------------------------------------------
bool SpoutFunctions::UpdateSender(const char* name, unsigned int width, unsigned int height)
{
	return spout.UpdateSender(name, width, height);
}

//---------------------------------------------------------
void SpoutFunctions::ReleaseSender(DWORD dwMsec)
{
	spout.ReleaseSender(dwMsec);
}

//---------------------------------------------------------
bool SpoutFunctions::SendTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, bool bInvert, GLuint HostFBO)
{
	return spout.SendTexture(TextureID, TextureTarget, width, height, bInvert, HostFBO);
}

/*
#ifdef legacyOpenGL
//---------------------------------------------------------
bool SpoutFunctions::DrawToSharedTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, float max_x, float max_y, float aspect, bool bInvert, GLuint HostFBO)
{
	return spout.DrawToSharedTexture(TextureID, TextureTarget, width, height, max_x, max_y, aspect, bInvert, HostFBO);
}
#endif
*/

//---------------------------------------------------------
bool SpoutFunctions::SendFboTexture(GLuint FboID, unsigned int width, unsigned int height, bool bInvert)
{
	return spout.SendFboTexture(FboID, width, height, bInvert);
}

//---------------------------------------------------------
bool SpoutFunctions::SendImage(const unsigned char* pixels, unsigned int width, unsigned int height, GLenum glFormat, bool bInvert, GLuint HostFBO)
{
	return spout.SendImage(pixels, width, height, glFormat, bInvert, HostFBO);
}

//---------------------------------------------------------
void SpoutFunctions::RemovePadding(const unsigned char *source, unsigned char *dest,
	unsigned int width, unsigned int height, unsigned int stride, GLenum glFormat)
{
	spout.RemovePadding(source, dest, width, height, stride, glFormat);
}

//---------------------------------------------------------
bool SpoutFunctions::CreateReceiver(char* name, unsigned int &width, unsigned int &height, bool bUseActive)
{
	return spout.CreateReceiver(name, width, height, bUseActive);
}

//---------------------------------------------------------
void SpoutFunctions::ReleaseReceiver()
{
	spout.ReleaseReceiver();
}

//---------------------------------------------------------
bool SpoutFunctions::ReceiveTexture(char* name, unsigned int &width, unsigned int &height, GLuint TextureID, GLuint TextureTarget, bool bInvert, GLuint HostFBO)
{
	return spout.ReceiveTexture(name, width, height, TextureID, TextureTarget, bInvert, HostFBO);
}

/*
#ifdef legacyOpenGL
//---------------------------------------------------------
bool SpoutFunctions::DrawSharedTexture(float max_x, float max_y, float aspect, bool bInvert, GLuint HostFBO)
{
	return spout.DrawSharedTexture(max_x, max_y, aspect, bInvert, HostFBO);
}
#endif
*/

//---------------------------------------------------------
bool SpoutFunctions::ReceiveImage(char* Sendername,
	unsigned int &width,
	unsigned int &height,
	unsigned char* pixels,
	GLenum glFormat,
	bool bInvert,
	GLuint HostFBO)
{
	return spout.ReceiveImage(Sendername, width, height, pixels, glFormat, bInvert, HostFBO);
}

//---------------------------------------------------------
bool SpoutFunctions::SelectSenderPanel(const char* message)
{
	return spout.SelectSenderPanel(message);
}

//---------------------------------------------------------
bool SpoutFunctions::CheckReceiver(char* name, unsigned int &width, unsigned int &height, bool &bConnected)
{
	return spout.CheckReceiver(name, width, height, bConnected);
}

//---------------------------------------------------------
bool SpoutFunctions::BindSharedTexture()
{
	return spout.BindSharedTexture();
}

//---------------------------------------------------------
bool SpoutFunctions::UnBindSharedTexture()
{
	return spout.UnBindSharedTexture();
}

//---------------------------------------------------------
int  SpoutFunctions::GetSenderCount()
{
	return spout.GetSenderCount();
}

//---------------------------------------------------------
bool SpoutFunctions::GetSender(int index, char* sendername, int MaxNameSize)
{
	return spout.GetSender(index, sendername, MaxNameSize);
}

//---------------------------------------------------------
bool SpoutFunctions::GetSenderInfo(const char* sendername, unsigned int &width, unsigned int &height, HANDLE &dxShareHandle, DWORD &dwFormat)
{
	return spout.GetSenderInfo(sendername, width, height, dxShareHandle, dwFormat);
}

//---------------------------------------------------------
bool SpoutFunctions::GetActiveSender(char* Sendername)
{
	return spout.GetActiveSender(Sendername);
}

//---------------------------------------------------------
bool SpoutFunctions::SetActiveSender(const char* Sendername)
{
	return spout.SetActiveSender(Sendername);
}

// Utilities

//---------------------------------------------------------
bool SpoutFunctions::GetDX9()
{
	return spout.interop.isDX9();
}

//---------------------------------------------------------
bool SpoutFunctions::SetDX9(bool bDX9)
{
	return spout.SetDX9(bDX9);
}

//---------------------------------------------------------
bool SpoutFunctions::GetMemoryShareMode()
{
	return spout.GetMemoryShareMode();
}

//---------------------------------------------------------
bool SpoutFunctions::SetMemoryShareMode(bool bMem)
{
	return spout.SetMemoryShareMode(bMem);
}

//---------------------------------------------------------
int SpoutFunctions::GetShareMode()
{
	return (spout.GetShareMode());
}

//---------------------------------------------------------
bool SpoutFunctions::SetShareMode(int mode)
{
	return (spout.SetShareMode(mode));
}

//---------------------------------------------------------
bool SpoutFunctions::GetBufferMode()
{
	return spout.GetBufferMode();
}

//---------------------------------------------------------
void SpoutFunctions::SetBufferMode(bool bActive)
{
	spout.SetBufferMode(bActive);
}

//---------------------------------------------------------
int SpoutFunctions::GetMaxSenders()
{
	// Get the maximum senders allowed from the sendernames class
	return(spout.interop.senders.GetMaxSenders());
}

//---------------------------------------------------------
void SpoutFunctions::SetMaxSenders(int maxSenders)
{
	// Sets the maximum senders allowed
	spout.interop.senders.SetMaxSenders(maxSenders);
}

//---------------------------------------------------------
bool SpoutFunctions::GetHostPath(const char* sendername, char* hostpath, int maxchars)
{
	return spout.GetHostPath(sendername, hostpath, maxchars);
}

//---------------------------------------------------------
bool SpoutFunctions::SetVerticalSync(bool bSync)
{
	return spout.interop.SetVerticalSync(bSync);
}

//---------------------------------------------------------
int SpoutFunctions::GetVerticalSync()
{
	return spout.interop.GetVerticalSync();
}

// Adapter functions

//---------------------------------------------------------
int SpoutFunctions::GetNumAdapters()
{
	return spout.GetNumAdapters();
}

//---------------------------------------------------------
bool SpoutFunctions::GetAdapterName(int index, char* adaptername, int maxchars)
{
	return spout.GetAdapterName(index, adaptername, maxchars);
}

//---------------------------------------------------------
bool SpoutFunctions::SetAdapter(int index)
{
	return spout.SetAdapter(index);
}

//---------------------------------------------------------
int SpoutFunctions::GetAdapter()
{
	return spout.GetAdapter();
}


// OpenGL

//---------------------------------------------------------
bool SpoutFunctions::CreateOpenGL()
{
	printf("SpoutFunctions::CreateOpenGL()\n");
	return spout.CreateOpenGL();
}

//---------------------------------------------------------
bool SpoutFunctions::CloseOpenGL()
{
	return spout.CloseOpenGL();
}

////////////////////////////////////////////////////////////////////////////////