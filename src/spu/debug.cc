/***************************************************************************
 *   Copyright (C) 2019 PCSX-Redux authors                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <SDL.h>
#include "imgui.h"

#include "spu/interface.h"

void PCSX::SPU::impl::debug() {
    uint32_t now = SDL_GetTicks();
    uint32_t delta = now - m_lastUpdated;
    while (delta >= 50) {
        m_lastUpdated += 50;
        delta -= 50;
        for (unsigned ch = 0; ch < MAXCHAN; ch++) {
            if (!s_chan[ch].bOn) {
                m_channelDebugTypes[ch][m_currentDebugSample] = EMPTY;
                m_channelDebugData[ch][m_currentDebugSample] = 0.0f;
            };
            if (s_chan[ch].iIrqDone) {
                m_channelDebugTypes[ch][m_currentDebugSample] = IRQ;
                m_channelDebugData[ch][m_currentDebugSample] = 0.0f;
                s_chan[ch].iIrqDone = 0;
                continue;
            }

            if (s_chan[ch].iMute) {
                m_channelDebugTypes[ch][m_currentDebugSample] = MUTED;
            } else if (s_chan[ch].bNoise) {
                m_channelDebugTypes[ch][m_currentDebugSample] = NOISE;
            } else if (s_chan[ch].bFMod == 1) {
                m_channelDebugTypes[ch][m_currentDebugSample] = FMOD1;
            } else if (s_chan[ch].bFMod == 2) {
                m_channelDebugTypes[ch][m_currentDebugSample] = FMOD2;
            } else {
                m_channelDebugTypes[ch][m_currentDebugSample] = DATA;
            }

            m_channelDebugData[ch][m_currentDebugSample] = fabsf((float)s_chan[ch].sval / 32768.0f);
        }
        if (++m_currentDebugSample == DEBUG_SAMPLES) m_currentDebugSample = 0;
    }
    if (!m_showDebug) return;
    ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 430), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("SPU Debug", &m_showDebug)) {
        ImGui::End();
        return;
    }
    {
        ImGui::BeginChild("##debugSPUleft", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0), true);
        ImGui::Columns(2);
        for (unsigned i = 0; i < MAXCHAN / 2; i++) {
            for (unsigned j = 0; j < 2; j++) {
                unsigned ch = j * MAXCHAN / 2 + i;
                std::string label1 = "##Channel" + std::to_string(ch);
                std::string label2 = "##Mute" + std::to_string(ch);
                std::string label3 = "Ch" + std::to_string(ch);
                ImGui::PlotHistogram(label1.c_str(), m_channelDebugData[ch], DEBUG_SAMPLES, 0, nullptr, 0.0f, 1.0f);
                ImGui::SameLine();
                ImGui::Checkbox(label2.c_str(), &s_chan[ch].iMute);
                ImGui::SameLine();
                if (ImGui::RadioButton(label3.c_str(), m_selectedChannel == ch)) m_selectedChannel = ch;
                ImGui::NextColumn();
            }
        }
        ImGui::Columns(1);
        if (ImGui::Button("Mute all", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
            for (unsigned ch = 0; ch < MAXCHAN; ch++) {
                s_chan[ch].iMute = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Unmute all", ImVec2(-1, 0))) {
            for (unsigned ch = 0; ch < MAXCHAN; ch++) {
                s_chan[ch].iMute = false;
            }
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();
    {
        auto ch = s_chan[m_selectedChannel];
        auto ADSRX = ch.ADSRX;

        ImGui::BeginChild("##debugSPUright", ImVec2(0, 0), true);
        {
            ImGui::Text("ADSR channel info");
            ImGui::Columns(2);
            {
                ImGui::Text("Attack:\nDecay:\nSustain:\nRelease:");
                ImGui::SameLine();
                ImGui::Text("%i\n%i\n%i\n%i", ADSRX.AttackRate ^ 0x7f, (ADSRX.DecayRate ^ 0x1f) / 4,
                            ADSRX.SustainRate ^ 0x7f, (ADSRX.ReleaseRate ^ 0x1f) / 4);
            }
            ImGui::NextColumn();
            {
                ImGui::Text("Sustain level:\nSustain inc:\nCurr adsr vol:\nRaw enveloppe");
                ImGui::SameLine();
                ImGui::Text("%i\n%i\n%i\n%08x", ADSRX.SustainLevel >> 27, ADSRX.SustainIncrease, ADSRX.lVolume,
                            ADSRX.EnvelopeVol);
            }
            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::Text("Generic channel info");
            ImGui::Columns(2);
            {
                ImGui::Text("On:\nStop:\nNoise:\nFMod:\nReverb:\nRvb active:\nRvb number:\nRvb offset:\nRvb repeat:");
                ImGui::SameLine();
                ImGui::Text("%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i", ch.bOn, ch.bStop, ch.bNoise, ch.bFMod, ch.bReverb,
                            ch.bRVBActive, ch.iRVBNum, ch.iRVBOffset, ch.iRVBRepeat);
            }
            ImGui::NextColumn();
            {
                ImGui::Text("Start pos:\nCurr pos:\nLoop pos:\n\nRight vol:\nLeft vol:\n\nAct freq:\nUsed freq:");
                ImGui::SameLine();
                ImGui::Text(
                    "%i\n%i\n%i\n\n%6i  %04x\n%6i  %04x\n\n%i\n%i", (unsigned long)ch.pStart - (unsigned long)spuMemC,
                    (unsigned long)ch.pCurr - (unsigned long)spuMemC, (unsigned long)ch.pLoop - (unsigned long)spuMemC,
                    ch.iRightVolume, ch.iRightVolRaw, ch.iLeftVolume, ch.iLeftVolRaw, ch.iActFreq, ch.iUsedFreq);
            }
            ImGui::Columns(1);
            ImGui::BeginChild("##debugSPUXA", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0), true);
            {
                ImGui::Text("XA");
                ImGui::Text("Freq:\nStereo:\nSamples:\nBuffered:\nVolume:\n");
                ImGui::SameLine();
                ImGui::Text("%i\n%i\n%i\n%i\n%5i  %5i", xapGlobal ? xapGlobal->freq : 0,
                            xapGlobal ? xapGlobal->stereo : 0, xapGlobal ? xapGlobal->nsamples : 0,
                            XAPlay <= XAFeed ? XAPlay - XAFeed : (XAFeed - XAStart) + (XAEnd - XAPlay), iLeftXAVol,
                            iRightXAVol);
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("##debugSPUstate", ImVec2(0, 0), true);
            {
                ImGui::Text("Spu states");
                ImGui::Text("Irq addr:\nCtrl:\nStat:\nSpu mem:");
                ImGui::SameLine();
                ImGui::Text("%i\n%04x\n%04x\n%i", pSpuIrq ? -1 : (unsigned long)pSpuIrq - (unsigned long)spuMemC,
                            spuCtrl, spuStat, spuAddr);
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }

    ImGui::End();
}
