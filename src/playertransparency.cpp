/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023-2024 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "string"
#include "common.h"
#include "playertransparency.h"
#include "playermanager.h"
#include "recipientfilters.h"
#include "commands.h"
#include "utils/entity.h"
#include "ctimer.h"
#include "entity/ccsplayercontroller.h"
#include "entity/ccsplayerpawn.h"

extern CGlobalVars* gpGlobals;

static std::string g_szTransparencyParticle = "";
FAKE_STRING_CVAR(cs2f_transparency_particle, "Path to particle being used as transparency proxy", g_szTransparencyParticle, false)

void Transparency_Precache(IEntityResourceManifest *pResourceManifest)
{
	if (!g_szTransparencyParticle.empty())
		pResourceManifest->AddResource(g_szTransparencyParticle.c_str());
}

void Transparency_OnRoundStart(IGameEvent* pEvent)
{
	Message("Round Started, reseting alpha on all players\n");
	// reset alpha for all players
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CCSPlayerController *pController = CCSPlayerController::FromSlot(i);

		if (!pController)
			continue;
		ZEPlayer *pPlayer = pController->GetZEPlayer();
		if (!pPlayer)
			return;
		pPlayer->MarkTransparent(false);
		pPlayer->SetTransparency(pPlayer->GetTransparencyPending());
		
		CCSPlayerPawn *pPawn = pController->GetPlayerPawn();
		if (pPawn)
		{
			UTIL_AddEntityIOEvent(pPawn, "Alpha", nullptr, nullptr, 0, 0.1);
			UTIL_AddEntityIOEvent(pPawn, "Alpha", nullptr, nullptr, 255, 0.2);
		}
	}
}

void Transparency_DispatchParticle(CCSPlayerPawn* pTarget)
{
	Message("Dispatching particle effect\n");
	CRecipientFilter filter;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		ZEPlayer *pPlayer = g_playerManager->GetPlayer(i);
		if (pPlayer && pPlayer->GetTransparency())
		{
			filter.AddRecipient(i);
		}
	}
	
	pTarget->DispatchParticle(g_szTransparencyParticle.c_str(), &filter);
}


void Transparency_OnPlayerSpawn(IGameEvent* pEvent)
{
	Message("player spawned\n");
	CCSPlayerController *pController = (CCSPlayerController*)pEvent->GetPlayerController("userid");
	if (!pController) // make sure it is only done once per round
		return;
	ZEPlayer *pPlayer = pController->GetZEPlayer();
	if (!pPlayer || pPlayer->IsTransparent())
	{
		Message("Already is transparent, skipping...\n");
		return;
	}
	CCSPlayerPawn *pPawn = (CCSPlayerPawn*) pController->GetPawn();
	if (!pPawn)
		return;

	CHandle<CCSPlayerPawn> pawnHandle = pPawn->GetHandle();
	new CTimer(0.21f, false, false, [pawnHandle]()
	{
		CCSPlayerPawn *pTarget = (CCSPlayerPawn*) pawnHandle.Get();
		if (pTarget)
			Transparency_DispatchParticle(pTarget);
		return -1.0f;
	});

	pPlayer->MarkTransparent(true);
}
CON_COMMAND_CHAT(pt, "toggle player transparency")
{
	if (!player)
		return;

	ZEPlayer *pZEPlayer = player->GetZEPlayer();
	if (!pZEPlayer)
		return;
	bool bTransparency = pZEPlayer->GetTransparencyPending();
	pZEPlayer->SetTransparencyPending(!bTransparency);
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Player transparency %s, effect will take place next round.", !bTransparency ? "enabled" : "disabled");
}