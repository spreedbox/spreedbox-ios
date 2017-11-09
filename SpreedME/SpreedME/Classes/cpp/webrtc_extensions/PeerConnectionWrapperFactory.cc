/**
 * @copyright Copyright (c) 2017 Struktur AG
 * @author Yuriy Shevchuk
 * @author Ivan Sein <ivan@nextcloud.com>
 *
 * @license GNU GPL version 3 or any later version
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "PeerConnectionWrapperFactory.h"

#include <assert.h>
#include <stdio.h>
#include <stdexcept>

#include <modules/audio_device/audio_device_impl.h>
#include <modules/video_capture/video_capture_factory.h>
#include "media/engine/webrtcvideocapturerfactory.h"
#include <common_video/video_render_frames.h>
#include <media/base/videosourceinterface.h>
#include <rtc_base/refcount.h>
#include <rtc_base/ssladapter.h>
#include <rtc_base/thread.h>
#include <pc/mediasession.h>

#include "utils.h"

using namespace spreedme;

PeerConnectionWrapperFactory::~PeerConnectionWrapperFactory()
{
    delete &_critSect;
	
	adm_ = NULL;
	peer_connection_factory_ = NULL;
	
	delete videoDeviceInfo_;

	if (videoConstraints_) {
		delete videoConstraints_;
	}
	if (audioConstraints_) {
		delete audioConstraints_;
	}
	
	delete worker_thread_;
	delete signaling_thread_;
}


PeerConnectionWrapperFactory::PeerConnectionWrapperFactory()  :
	_critSect(*webrtc::RWLockWrapper::CreateRWLock()),
	worker_thread_(NULL),
	signaling_thread_(NULL),
	audioConstraints_(NULL),
	videoConstraints_(NULL),
	identifierBase_(0)
{
	bool initialized = rtc::InitializeSSL();
	if (!initialized) {
		throw std::runtime_error("SSL is not initialized! Normal work is no longer possible.");
	}
	//NSAssert(initialized, @"Failed to initialize SSL library");
	
	this->InternalInitializeThreads();
	
    adm_ = rtc::scoped_refptr<webrtc::AudioDeviceModule>(webrtc::AudioDeviceModule::Create(0, webrtc::AudioDeviceModule::kPlatformDefaultAudio));
	
	
	// _adm should be initialized before initializing PeerConnectionFactory since latter uses _adm in init
	bool success = this->InitializePeerConnectionFactory();
	if (!success) {
		throw std::runtime_error("PeerConnectionFactory is not initialized! Normal work is no longer possible.");
	}
}


void PeerConnectionWrapperFactory::InternalInitializeThreads()
{
	if (!worker_thread_ && !signaling_thread_) {
		worker_thread_ = new rtc::Thread();
		worker_thread_->SetName("Worker_Thread", worker_thread_);
		signaling_thread_ = new rtc::Thread();
		signaling_thread_->SetName("Signalling_Thread", signaling_thread_);
	
		worker_thread_->Start();
		signaling_thread_->Start();
	} else {
		spreed_me_log("worker_thread or signaling_thread or both are initialized.");
	}
}


bool PeerConnectionWrapperFactory::InitializePeerConnectionFactory()
{
	peer_connection_factory_  = webrtc::CreatePeerConnectionFactory(
																	worker_thread_,
																	signaling_thread_,
																	adm_,
																	NULL,
																	NULL
																	);
	webrtc::PeerConnectionFactoryInterface::Options options = webrtc::PeerConnectionFactoryInterface::Options();
	options.disable_encryption = false;
	options.disable_sctp_data_channels = false;
	peer_connection_factory_->SetOptions(options);
	
    if (!peer_connection_factory_.get()) {
        spreed_me_log("Failed to initialize PeerConnectionFactory\n");
		peer_connection_factory_ = NULL;
        return false;
    }
	
	/*deviceManager_ = rtc::scoped_ptr<cricket::DeviceManagerInterface>(cricket::DeviceManagerFactory::Create());
	bool initialized = deviceManager_->Init();
	if (!initialized) {
		spreed_me_log("Couldn't initialize video device manager!");
	}*/
	
	videoDeviceInfo_ = webrtc::videocapturemodule::VideoCaptureImpl::CreateDeviceInfo();
	if (videoDeviceInfo_ == NULL) {
		spreed_me_log("Couldn't initialize video device info!");
	}
    
	return true;
}


std::string PeerConnectionWrapperFactory::GetNewSpreedPeerConnectionId()
{
	std::string id;
	std::stringstream strstream;
	strstream << identifierBase_;
	strstream >> id;
	
	/* Since we can't have that much participants at once we can safely increase identifierBase_ and wrap it when it is ULONG_LONG_MAX.
	 It is extremely unlikely that we would still have connections with ids around zero when identifierBase_ will be around ULONG_LONG_MAX. 
	 If that happens I would consider it a bug.*/
	if (identifierBase_ < 4564564) {
		++identifierBase_;
	} else {
		identifierBase_ = 0;
	}
	
	return id;
}


rtc::scoped_refptr<PeerConnectionWrapper>
PeerConnectionWrapperFactory::CreateSpreedPeerConnection(const std::string &userId,
														 PeerConnectionWrapperDelegateInterface *pcDelegate)
{
	std::string id = this->GetNewSpreedPeerConnectionId();
	rtc::scoped_refptr<PeerConnectionWrapper> peerConnectionWrapper(new rtc::RefCountedObject<PeerConnectionWrapper>(id, pcDelegate));
	
	MediaConstraints connectionConstraints;
	connectionConstraints.AddMandatory(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, webrtc::MediaConstraintsInterface::kValueTrue);
	connectionConstraints.AddMandatory(webrtc::MediaConstraintsInterface::kEnableRtpDataChannels, webrtc::MediaConstraintsInterface::kValueFalse);
	
	peerConnectionWrapper->SetConnectionConstraints(connectionConstraints);
	
	webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
	rtc_config.servers = iceServers_;
	
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection =
		peer_connection_factory_->CreatePeerConnection(rtc_config,
													   peerConnectionWrapper->connectionConstraintsRef(),
													   NULL,
													   NULL,
													   peerConnectionWrapper);
    if (!peerConnection.get()) {
        spreed_me_log("CreatePeerConnection failed\n");
        peerConnectionWrapper = NULL;
        return NULL;
    }
	
	peerConnectionWrapper->SetPeerConnection(peerConnection);
	peerConnectionWrapper->SetUserId(userId);
	
    return peerConnectionWrapper;
}

std::unique_ptr<cricket::VideoCapturer>
PeerConnectionWrapperFactory::OpenVideoCaptureDevice() {
    std::vector<std::string> device_names;
    {
        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
                                                                     webrtc::VideoCaptureFactory::CreateDeviceInfo());
        if (!info) {
            return nullptr;
        }
        int num_devices = info->NumberOfDevices();
        for (int i = 0; i < num_devices; ++i) {
            const uint32_t kSize = 256;
            char name[kSize] = {0};
            char id[kSize] = {0};
            if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
                device_names.push_back(name);
            }
        }
    }
    
    cricket::WebRtcVideoDeviceCapturerFactory factory;
    std::unique_ptr<cricket::VideoCapturer> capturer;
    for (const auto& name : device_names) {
        capturer = factory.Create(cricket::Device(name, 0));
        if (capturer) {
            break;
        }
    }
    return capturer;
}

rtc::scoped_refptr<webrtc::MediaStreamInterface> PeerConnectionWrapperFactory::CreateLocalStream(bool withAudio, bool withVideo)
{
	std::string streamLabel;
	std::string audioTrackId;
	std::string videoTrackId;
	
	int streamIdLength = 16;
	bool succes = rtc::CreateRandomString(streamIdLength, &streamLabel);
	if (!succes) {
		spreed_me_log("Couldn't generate random string to create stream label!\n");
		assert(false);
	}
	
	int trackIdLength = 25;
	succes = rtc::CreateRandomString(trackIdLength, &audioTrackId);
	if (!succes) {
		spreed_me_log("Couldn't generate random string to create audioTrackId!\n");
		assert(false);
	}
	audioTrackId = std::string("at_") + audioTrackId;
	
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =	peer_connection_factory_->CreateLocalMediaStream(streamLabel);
	
	if (audioSource_ && withAudio) {
		rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(peer_connection_factory_->CreateAudioTrack(audioTrackId, audioSource_));
		
		stream->AddTrack(audio_track);
	} else {
		spreed_me_log("No audioSource_!");
	}
	
	if (!videoDeviceId_.empty() && withVideo) {
	
		succes = rtc::CreateRandomString(trackIdLength, &videoTrackId);
		if (!succes) {
			spreed_me_log("Couldn't generate random string to create videoTrackId!\n");
			assert(false);
		}
		videoTrackId = std::string("vt_") + videoTrackId;
    
        if (!videoSource_) {
                //capturer_ = OpenVideoCaptureDevice();
                videoSource_ = peer_connection_factory_->CreateVideoSource(OpenVideoCaptureDevice(), videoConstraints_);
            }
            rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(peer_connection_factory_->CreateVideoTrack(videoTrackId, videoSource_.get()));
            stream->AddTrack(video_track);
        }

	
    return stream;
}


void PeerConnectionWrapperFactory::SetVideoDeviceId(const std::string &videoDeviceId)
{
	videoDeviceId_ = videoDeviceId;
}


void PeerConnectionWrapperFactory::SetAudioVideoConstrains(MediaConstraints *audioSourceConstraints, MediaConstraints *videoSourceConstraints)
{
	if (audioConstraints_) {
		delete audioConstraints_;
	}
	audioConstraints_ = audioSourceConstraints;
	audioSource_ = peer_connection_factory_->CreateAudioSource(audioConstraints_);
	
	if (videoConstraints_) {
		delete videoConstraints_;
	}
	videoConstraints_ = videoSourceConstraints;
	this->DisposeOfVideoSource();
}


void PeerConnectionWrapperFactory::DisposeOfVideoSource()
{
	if (videoSource_) {
		//capturer_.get()->Stop();
        //capturer_ = NULL;
	}
	videoSource_ = NULL;
}


void PeerConnectionWrapperFactory::StopVideoCapturing()
{
	if (videoSource_) {
		/*cricket::VideoCapturer *videoCapturer = capturer_.get();
		const cricket::VideoFormat *videoFormat = videoCapturer->GetCaptureFormat();
		if (videoFormat) {
			currentCaptureFormat_ = *videoFormat;
		} else {
			currentCaptureFormat_ = cricket::VideoFormat();
		}
		capturer_.get()->Stop();*/
	}
}


void PeerConnectionWrapperFactory::StartVideoCapturing()
{
	if (videoSource_) {
		/*cricket::VideoCapturer *videoCapturer = capturer_.get();
		if (currentCaptureFormat_.width == 0 && currentCaptureFormat_.height == 0 &&
			currentCaptureFormat_.interval == 0 && currentCaptureFormat_.fourcc == 0) {
			spreed_me_log("We can't restart video capturer! We have no capturing format");
		} else {
			videoCapturer->StartCapturing(currentCaptureFormat_);
		}*/
	}
}


STDStringVector PeerConnectionWrapperFactory::videoDeviceUniqueIDs()
{
	STDStringVector videoDeviceUniqueIDs;
    //videoDeviceUniqueIDs.push_back(capturer_.get()->GetId());
  
	return videoDeviceUniqueIDs;
}


std::vector<webrtc::VideoCaptureCapability> PeerConnectionWrapperFactory::GetVideoDeviceCaptureCapabilities(const std::string &videoDeviceUniqueId)
{
	std::vector<webrtc::VideoCaptureCapability> capabilities;
	
	if (videoDeviceInfo_) {
		int32_t numberOfCapabilities = videoDeviceInfo_->NumberOfCapabilities(videoDeviceUniqueId.c_str());
		
		for (int32_t i = 0; i < numberOfCapabilities; i++) {
			webrtc::VideoCaptureCapability capability;
			int32_t result = videoDeviceInfo_->GetCapability(videoDeviceUniqueId.c_str(), i, capability);
			if (result == 0) {
				capabilities.push_back(capability);
			}
		}
	}
	
	return capabilities;
}


std::string PeerConnectionWrapperFactory::GetLocalizedNameOfVideoDevice(const std::string &videoDeviceUniqueId)
{
	std::string deviceName;
	
    //deviceName = capturer_.get()->GetId();
	
	return deviceName;
}


void PeerConnectionWrapperFactory::SetMuteAudio(bool mute)
{
	adm_->SetMicrophoneMute(mute);
}


void PeerConnectionWrapperFactory::SetSpeakerPhone(bool yesNo)
{
	adm_->SetLoudspeakerStatus(yesNo);
}


void PeerConnectionWrapperFactory::AudioInterruptionStarted()
{
	
}


void PeerConnectionWrapperFactory::AudioInterruptionStopped()
{
	//adm_->ResetAudioDevice();
}
