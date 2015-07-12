#include "stdafx.h"
#include "XAudio2VoiceProxy.h"

#include "XAPO/HrtfEffect.h"
#include <XAPO.h>
#include <XAPOFX.h>

#include "util.h"
#include "logger.h"
#include <cmath>

XAudio2VoiceProxy::XAudio2VoiceProxy(const std::wstring & type_name, const IVoiceMapper * voice_mapper, IXAudio2Voice * original, const void * id)
	: m_original(original)
	, m_id(id)
	, m_type_name(type_name)
	, m_sound3d_registry(nullptr)
	, m_voice_mapper(voice_mapper)
	, m_input_channels(0)
	, m_hrtf_effect_index(UINT_MAX)
	, m_is_master(true)
{
}

XAudio2VoiceProxy::XAudio2VoiceProxy(const std::wstring & type_name, ISound3DRegistry * sound3d_registry, const IVoiceMapper * voice_mapper, IXAudio2Voice * original, UINT32 input_channels, const void * id, UINT32 hrtf_effect_index)
	: m_original(original)
	, m_id(id)
	, m_type_name(type_name)
	, m_sound3d_registry(sound3d_registry)
	, m_voice_mapper(voice_mapper)
	, m_input_channels(input_channels)
	, m_hrtf_effect_index(hrtf_effect_index)
	, m_is_master(false)
{
	//AlterAndSetEffectChain(original_chain);
}

void XAudio2VoiceProxy::GetVoiceDetails(XAUDIO2_VOICE_DETAILS * pVoiceDetails)
{
	m_original->GetVoiceDetails(pVoiceDetails);
}

HRESULT XAudio2VoiceProxy::SetOutputVoices(const XAUDIO2_VOICE_SENDS * pSendList)
{
	std::wstringstream ss;
	ss << m_type_name << "::SetOutputVoices " << m_id << " ";
	print_sends(ss, pSendList);
	logger::log(ss.str());

	XAUDIO2_VOICE_SENDS originalSendList = { 0 };
	if (pSendList)
		m_voice_mapper->MapSendsToOriginal(*pSendList, originalSendList);

	auto result = m_original->SetOutputVoices(pSendList ? &originalSendList : 0);

	m_voice_mapper->CleanupSends(originalSendList);

	return result;
}

HRESULT XAudio2VoiceProxy::SetEffectChain(const XAUDIO2_EFFECT_CHAIN * pEffectChain)
{
	// Not supported right now
	return S_OK;
	//return m_original->SetEffectChain(pEffectChain);
	//return AlterAndSetEffectChain(pEffectChain);
}

HRESULT XAudio2VoiceProxy::EnableEffect(UINT32 EffectIndex, UINT32 OperationSet)
{
	return S_OK;
	//Not supported now
	//return m_original->EnableEffect(EffectIndex, OperationSet);
}

HRESULT XAudio2VoiceProxy::DisableEffect(UINT32 EffectIndex, UINT32 OperationSet)
{
	return S_OK;
	//Not supported now
	//return m_original->DisableEffect(EffectIndex, OperationSet);
}

void XAudio2VoiceProxy::GetEffectState(UINT32 EffectIndex, BOOL * pEnabled)
{
	m_original->GetEffectState(EffectIndex, pEnabled);
}

HRESULT XAudio2VoiceProxy::SetEffectParameters(UINT32 EffectIndex, const void * pParameters, UINT32 ParametersByteSize, UINT32 OperationSet)
{
	return S_OK;

	// Not supported right now
	//return m_original->SetEffectParameters(EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

HRESULT XAudio2VoiceProxy::GetEffectParameters(UINT32 EffectIndex, void * pParameters, UINT32 ParametersByteSize)
{
	return S_OK;
	// Not supported right now
	//return m_original->GetEffectParameters(EffectIndex, pParameters, ParametersByteSize);
}

HRESULT XAudio2VoiceProxy::SetFilterParameters(const XAUDIO2_FILTER_PARAMETERS * pParameters, UINT32 OperationSet)
{
	return m_original->SetFilterParameters(pParameters, OperationSet);
}

void XAudio2VoiceProxy::GetFilterParameters(XAUDIO2_FILTER_PARAMETERS * pParameters)
{
	m_original->GetFilterParameters(pParameters);
}

HRESULT XAudio2VoiceProxy::SetOutputFilterParameters(IXAudio2Voice * pDestinationVoice, const XAUDIO2_FILTER_PARAMETERS * pParameters, UINT32 OperationSet)
{
	return m_original->SetOutputFilterParameters(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), pParameters, OperationSet);
}

void XAudio2VoiceProxy::GetOutputFilterParameters(IXAudio2Voice * pDestinationVoice, XAUDIO2_FILTER_PARAMETERS * pParameters)
{
	m_original->GetOutputFilterParameters(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), pParameters);
}

HRESULT XAudio2VoiceProxy::SetVolume(float Volume, UINT32 OperationSet)
{
	return m_original->SetVolume(Volume, OperationSet);
}

void XAudio2VoiceProxy::GetVolume(float * pVolume)
{
	m_original->GetVolume(pVolume);
}

HRESULT XAudio2VoiceProxy::SetChannelVolumes(UINT32 Channels, const float * pVolumes, UINT32 OperationSet)
{
	logger::log(m_type_name, "::SetChannelVolumes Channels=", Channels, " m_output_channels=", m_output_channels);
	return m_original->SetChannelVolumes(Channels, pVolumes, OperationSet);
}

void XAudio2VoiceProxy::GetChannelVolumes(UINT32 Channels, float * pVolumes)
{
	m_original->GetChannelVolumes(Channels, pVolumes);
}


HRESULT XAudio2VoiceProxy::SetOutputMatrix(IXAudio2Voice * pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, const float * pLevelMatrix, UINT32 OperationSet)
{
	//return m_original->SetOutputMatrix(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), SourceChannels, DestinationChannels, pLevelMatrix);

	if (std::isnan(pLevelMatrix[0]))
	{
		logger::log(m_type_name, "::SetOutputMatrix ", m_id, " Has NaN and DestinationChannels=", DestinationChannels, " m_output_channels=", m_output_channels, " SourceChannels=", SourceChannels);
	}

	if (std::isnan(pLevelMatrix[0]))
	{
		if (DestinationChannels == 2)
		{
			//if (m_input_channels == 2)
			m_original->EnableEffect(m_hrtf_effect_index, OperationSet);

			//auto & id = *reinterpret_cast<const sound_id*>(&pLevelMatrix[1]);
			//auto sound3d = m_sound3d_registry->GetEntry(id);

			float matrix[] = { 0.1f, 0.0f, 0.0f, 0.1f };
			return m_original->SetOutputMatrix(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), 2, 2, matrix, OperationSet);
		}
		else
		{
			return E_FAIL;
		}
	}
	else
	{
		m_original->DisableEffect(m_hrtf_effect_index, OperationSet);

		if (SourceChannels == 1)
		{
			float matrix[XAUDIO2_MAX_AUDIO_CHANNELS] = { 0 };
			for (UINT32 i = 0; i < DestinationChannels; i++)
			{
				matrix[i * 2] = matrix[i * 2 + 1] = pLevelMatrix[i];
			}
			return m_original->SetOutputMatrix(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), 2, DestinationChannels, matrix, OperationSet);
		}
		else if (SourceChannels == 2)
		{
			return m_original->SetOutputMatrix(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), 2, DestinationChannels, pLevelMatrix, OperationSet);
		}
		else
		{
			return E_FAIL;
			// else is not supported
		}
	}
}

void XAudio2VoiceProxy::GetOutputMatrix(IXAudio2Voice * pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, float * pLevelMatrix)
{
	m_original->GetOutputMatrix(m_voice_mapper->MapVoiceToOriginal(pDestinationVoice), SourceChannels, DestinationChannels, pLevelMatrix);
}

HRESULT XAudio2VoiceProxy::AlterAndSetEffectChain(const XAUDIO2_EFFECT_CHAIN * original_chain)
{
	std::wstringstream ss;
	if (original_chain)
	{
		ss << "[";
		for (UINT32 i = 0; i < original_chain->EffectCount; i++)
		{
			IXAPO * effect;
			original_chain->pEffectDescriptors[i].pEffect->QueryInterface(&effect);
			XAPO_REGISTRATION_PROPERTIES * props;
			effect->GetRegistrationProperties(&props);
			ss << props->FriendlyName;
			if (i < original_chain->EffectCount - 1)
				ss << ", ";
			XAPOFree(props);
		}
		ss << "]";
	}
	logger::log(m_type_name, "::AlterEffectChain", " ", m_id, " original_chain=", original_chain, (original_chain != nullptr ? L" count=" + std::to_wstring(original_chain->EffectCount) : L" none"), " ", ss.str());


	UINT32 effect_count = (original_chain != nullptr) ? original_chain->EffectCount : 0;
	_ASSERT(effect_count < 64);

	m_output_channels = (effect_count > 0) ? original_chain->pEffectDescriptors[effect_count - 1].OutputChannels : m_input_channels;

	if (has_hrtf_effect())
	{
		logger::log("AlterEffectChain actually altering");

		XAUDIO2_EFFECT_DESCRIPTOR apoDesc[64];
		for (UINT32 i = 0; i < effect_count; i++)
		{
			apoDesc[i] = original_chain->pEffectDescriptors[i];
		}

		IUnknown * apo;
		//CreateFX(__uuidof(FXEQ), &apo);
		apo = static_cast<IUnknown *>(static_cast<IXAPOParameters *>(HrtfXapoEffect::CreateInstance()));
		//HrtfXapoEffect::CreateInstance()->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&apo));

		apoDesc[effect_count].InitialState = false;
		apoDesc[effect_count].OutputChannels = 2;
		apoDesc[effect_count].pEffect = apo;

		XAUDIO2_EFFECT_CHAIN chain;
		chain.EffectCount = effect_count + 1;
		chain.pEffectDescriptors = apoDesc;

		m_hrtf_effect_index = effect_count;

		//m_original->SetEffectChain(nullptr);
		auto result = m_original->SetEffectChain(&chain);
		apo->Release();
		return result;
		//return S_OK;
	}
	else
	{
		return m_original->SetEffectChain(original_chain);
	}
}

bool XAudio2VoiceProxy::has_hrtf_effect() const
{
	return m_hrtf_effect_index != UINT32_MAX;
}