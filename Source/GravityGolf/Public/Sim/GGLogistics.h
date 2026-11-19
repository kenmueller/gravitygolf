// Carrying. A grille is a gate that, while shut, stops a carried machine as well as the ball (the player alone walks
// through it, as through any gate). On a hole with grilles, getting the machines to where the answer needs them is a
// puzzle of its own: the weight that holds a grille open is itself a machine, and lifting it shuts the grille.
//
// This file answers "can the machines be walked from here to there, and by which carries?" for the proof, the rival
// and the demo. Plain C++ like the simulation. It works on holes with plinths, where a machine has only so many
// places to stand: a state is which plinth each machine is on, and a move is one carry.

#pragma once

#include "Sim/GGProof.h"

#include <cstdint>
#include <map>
#include <queue>
#include <vector>

namespace GG
{
/** One carry: lift this machine from one plinth and set it on another. */
struct FCarry
{
	int Machine = 0;
	int From = 0;
	int To = 0;
};

/** The ground as a walker with a machine in its arms meets it: a grid of squares, some walled, some under a grille. */
struct FCarryMap
{
	double MinX = 0, MinY = 0, Cell = 25.0;
	int Width = 0, Height = 0;
	/** 1 where a wall (or the edge of the hole) stops anyone. */
	std::vector<uint8_t> Wall;
	/** Per gate: the squares it bars while shut, if it is a grille (empty for an ordinary gate). */
	std::vector<std::vector<int>> Barred;
	/** The square each plinth stands in. */
	std::vector<int> PlinthCell;

	int At(double X, double Y) const
	{
		const int Cx = static_cast<int>(std::floor((X - MinX) / Cell));
		const int Cy = static_cast<int>(std::floor((Y - MinY) / Cell));
		return Cx < 0 || Cy < 0 || Cx >= Width || Cy >= Height ? -1 : Cy * Width + Cx;
	}
};

/**
 * Body is how far a walker's middle stays from a wall. Anything standing more than knee high is a wall; the ground
 * outside the hole's bounds is too (a hole with grilles has to be walled in, or its machines are carried round).
 */
inline FCarryMap MakeCarryMap(const FSetup& S, double Body = 35.0)
{
	FCarryMap Map;
	Map.MinX = S.MinX - 100.0;
	Map.MinY = S.MinY - 100.0;
	Map.Width = static_cast<int>(std::ceil((S.MaxX - S.MinX + 200.0) / Map.Cell));
	Map.Height = static_cast<int>(std::ceil((S.MaxY - S.MinY + 200.0) / Map.Cell));
	Map.Wall.assign(static_cast<size_t>(Map.Width) * Map.Height, 0);
	auto Cover = [&](const FBox& Box, double Grow, auto&& Mark) {
		const int X0 = static_cast<int>(std::floor((Box.Min.X - Grow - Map.MinX) / Map.Cell));
		const int X1 = static_cast<int>(std::floor((Box.Max.X + Grow - Map.MinX) / Map.Cell));
		const int Y0 = static_cast<int>(std::floor((Box.Min.Y - Grow - Map.MinY) / Map.Cell));
		const int Y1 = static_cast<int>(std::floor((Box.Max.Y + Grow - Map.MinY) / Map.Cell));
		for (int Y = Y0 < 0 ? 0 : Y0; Y <= Y1 && Y < Map.Height; ++Y)
		{
			for (int X = X0 < 0 ? 0 : X0; X <= X1 && X < Map.Width; ++X)
			{
				Mark(Y * Map.Width + X);
			}
		}
	};
	const double Ground = S.Course.Tee.Z - S.Course.BallRadius;
	for (const FBox& Box : S.Course.Boxes)
	{
		if (Box.Max.Z > Ground + 50.0 && Box.Min.Z < Ground + 150.0)
		{
			Cover(Box, Body, [&](int Cell) { Map.Wall[Cell] = 1; });
		}
	}
	Map.Barred.resize(S.Course.Gates.size());
	for (size_t G = 0; G < S.Course.Gates.size(); ++G)
	{
		if (S.Course.Gates[G].bGrille)
		{
			Cover(S.Course.Gates[G].Box, Body, [&](int Cell) { Map.Barred[G].push_back(Cell); });
		}
	}
	for (const FVec3& Plinth : S.Plinths)
	{
		Map.PlinthCell.push_back(Map.At(Plinth.X, Plinth.Y));
	}
	return Map;
}

/** The plinth a machine at (X, Y) stands on, or -1. */
inline int PlinthAt(const FSetup& S, double X, double Y)
{
	for (size_t P = 0; P < S.Plinths.size(); ++P)
	{
		const double Dx = X - S.Plinths[P].X, Dy = Y - S.Plinths[P].Y;
		if (Dx * Dx + Dy * Dy <= S.Plinths[P].Z * S.Plinths[P].Z)
		{
			return static_cast<int>(P);
		}
	}
	return -1;
}

/**
 * Which switches are down while machine Lifted is in someone's arms (-1: nobody is carrying): the ones banked by
 * the ball on earlier legs, and every weight plate one of the other machines stands on.
 */
inline std::vector<bool> HeldWhileCarrying(const FSetup& S, const std::vector<int>& On, int Lifted, const std::vector<bool>& Banked)
{
	std::vector<bool> Held = Banked;
	Held.resize(S.Course.Switches.size(), false);
	std::vector<FAttractor> Standing;
	for (size_t M = 0; M < On.size(); ++M)
	{
		if (static_cast<int>(M) != Lifted && On[M] >= 0)
		{
			Standing.push_back(Stand(S.Machines[M], S.Plinths[On[M]].X, S.Plinths[On[M]].Y));
		}
	}
	HoldWeightPlates(S.Course, Standing, Held);
	return Held;
}

/** Every plinth a machine lifted at plinth From can be walked to, given which switches are down. */
inline std::vector<bool> CarryReach(const FSetup& S, const FCarryMap& Map, int From, const std::vector<bool>& Held)
{
	std::vector<uint8_t> Shut = Map.Wall;
	for (size_t G = 0; G < Map.Barred.size(); ++G)
	{
		if (!Held[S.Course.Gates[G].Switch])
		{
			for (const int Cell : Map.Barred[G])
			{
				Shut[Cell] = 1;
			}
		}
	}
	std::vector<bool> Reached(S.Plinths.size(), false);
	const int Start = Map.PlinthCell[From];
	if (Start < 0)
	{
		return Reached;
	}
	std::vector<uint8_t> Seen(Shut.size(), 0);
	std::queue<int> Open;
	Open.push(Start);
	Seen[Start] = 1;
	while (!Open.empty())
	{
		const int Cell = Open.front();
		Open.pop();
		const int X = Cell % Map.Width, Y = Cell / Map.Width;
		const int Next[4] = {X > 0 ? Cell - 1 : -1, X + 1 < Map.Width ? Cell + 1 : -1, Y > 0 ? Cell - Map.Width : -1,
			Y + 1 < Map.Height ? Cell + Map.Width : -1};
		for (const int N : Next)
		{
			if (N >= 0 && !Seen[N] && !Shut[N])
			{
				Seen[N] = 1;
				Open.push(N);
			}
		}
	}
	for (size_t P = 0; P < S.Plinths.size(); ++P)
	{
		Reached[P] = Map.PlinthCell[P] >= 0 && Seen[Map.PlinthCell[P]];
	}
	return Reached;
}

/**
 * Every way the machines can come to stand, starting from Start (a plinth per machine), with the carries that get
 * there in the fewest moves. Two machines never share a plinth.
 */
struct FCarryPlans
{
	/** A way of standing, as one number, to how it was reached: the way before it and the carry between. */
	std::map<long long, std::pair<long long, FCarry>> Reached;
	int Plinths = 0;

	long long Key(const std::vector<int>& On) const
	{
		long long K = 0;
		for (const int P : On)
		{
			K = K * (Plinths + 1) + (P + 1);
		}
		return K;
	}
	bool Can(const std::vector<int>& On) const { return Reached.count(Key(On)) != 0; }
	/** The carries from the start to this way of standing, in order (empty if it is the start, or cannot be reached). */
	std::vector<FCarry> To(const std::vector<int>& On) const
	{
		std::vector<FCarry> Moves;
		long long K = Key(On);
		for (auto It = Reached.find(K); It != Reached.end() && It->second.first != K; It = Reached.find(K))
		{
			Moves.insert(Moves.begin(), It->second.second);
			K = It->second.first;
		}
		return Moves;
	}
};

inline FCarryPlans PlanCarries(const FSetup& S, const FCarryMap& Map, const std::vector<int>& Start, const std::vector<bool>& Banked)
{
	FCarryPlans Plans;
	Plans.Plinths = static_cast<int>(S.Plinths.size());
	std::queue<std::vector<int>> Open;
	Plans.Reached[Plans.Key(Start)] = {Plans.Key(Start), FCarry{}};
	Open.push(Start);
	while (!Open.empty())
	{
		const std::vector<int> On = Open.front();
		Open.pop();
		for (size_t M = 0; M < On.size(); ++M)
		{
			if (On[M] < 0)
			{
				continue;
			}
			const std::vector<bool> Reach = CarryReach(S, Map, On[M], HeldWhileCarrying(S, On, static_cast<int>(M), Banked));
			for (int P = 0; P < Plans.Plinths; ++P)
			{
				bool bFree = Reach[P] && P != On[M];
				for (size_t Other = 0; Other < On.size() && bFree; ++Other)
				{
					bFree = Other == M || On[Other] != P;
				}
				if (!bFree)
				{
					continue;
				}
				std::vector<int> Next = On;
				Next[M] = P;
				if (!Plans.Can(Next))
				{
					Plans.Reached[Plans.Key(Next)] = {Plans.Key(On), FCarry{static_cast<int>(M), On[M], P}};
					Open.push(Next);
				}
			}
		}
	}
	return Plans;
}
} // namespace GG
