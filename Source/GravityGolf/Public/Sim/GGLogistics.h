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
#include <unordered_map>
#include <utility>
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
	/** 1 where a walker's middle cannot be: in a wall, or nearer to one than half a body. */
	std::vector<uint8_t> Wall;
	/** 1 where the wall itself stands (or, on a drawn hole, where there is no ground): nothing is handed across it. */
	std::vector<uint8_t> Solid;
	/** Per gate: the squares it bars while shut, if it is a grille (empty for an ordinary gate). */
	std::vector<std::vector<int>> Barred;
	/** The square each plinth stands in. */
	std::vector<int> PlinthCell;
	/** Worked out as it is asked for: for one state of the grilles (shut or not, gate by gate), a number for each
	 *  plinth, the same for two plinths a machine can be walked between. (The planner asks thousands of times, for a
	 *  handful of states. Not for use from two threads at once.) */
	mutable std::map<std::vector<bool>, std::vector<int>> Groups;

	int At(double X, double Y) const
	{
		const int Cx = static_cast<int>(std::floor((X - MinX) / Cell));
		const int Cy = static_cast<int>(std::floor((Y - MinY) / Cell));
		return Cx < 0 || Cy < 0 || Cx >= Width || Cy >= Height ? -1 : Cy * Width + Cx;
	}
};

/** Is this box a wall to someone on foot? Anything standing more than knee high. */
inline bool IsCarryWall(const FCourse& Course, const FBox& Box)
{
	const double Ground = Course.Tee.Z - Course.BallRadius;
	return Box.Max.Z > Ground + 50.0 && Box.Min.Z < Ground + 150.0;
}

/** Is this box ground: is its top where the ball rolls? */
inline bool IsCarryGround(const FCourse& Course, const FBox& Box)
{
	const double Ground = Course.Tee.Z - Course.BallRadius;
	return Box.Max.Z >= Ground - 1.0 && Box.Max.Z <= Ground + 1.0;
}

/**
 * Body is how far a walker's middle stays from a wall. A square belongs to a wall when its middle does. The ground
 * outside the hole's bounds is wall too (a hole with grilles has to be walled in, or its machines are carried round),
 * and on a hole drawn in the Workshop (bGroundOnly) so is every square with none of the hole's ground under it.
 */
inline FCarryMap MakeCarryMap(const FSetup& S, double Body = 35.0)
{
	FCarryMap Map;
	Map.MinX = S.MinX - 100.0;
	Map.MinY = S.MinY - 100.0;
	Map.Width = static_cast<int>(std::ceil((S.MaxX - S.MinX + 200.0) / Map.Cell));
	Map.Height = static_cast<int>(std::ceil((S.MaxY - S.MinY + 200.0) / Map.Cell));
	Map.Wall.assign(static_cast<size_t>(Map.Width) * Map.Height, 0);
	Map.Solid = Map.Wall;
	// Every square whose middle is inside the box grown by Grow.
	auto Cover = [&](const FBox& Box, double Grow, auto&& Mark) {
		const int X0 = static_cast<int>(std::ceil((Box.Min.X - Grow - Map.MinX) / Map.Cell - 0.5));
		const int X1 = static_cast<int>(std::floor((Box.Max.X + Grow - Map.MinX) / Map.Cell - 0.5));
		const int Y0 = static_cast<int>(std::ceil((Box.Min.Y - Grow - Map.MinY) / Map.Cell - 0.5));
		const int Y1 = static_cast<int>(std::floor((Box.Max.Y + Grow - Map.MinY) / Map.Cell - 0.5));
		for (int Y = Y0 < 0 ? 0 : Y0; Y <= Y1 && Y < Map.Height; ++Y)
		{
			for (int X = X0 < 0 ? 0 : X0; X <= X1 && X < Map.Width; ++X)
			{
				Mark(Y * Map.Width + X);
			}
		}
	};
	if (S.bGroundOnly)
	{
		// Everything is shut out except the squares over the hole's ground.
		Map.Solid.assign(Map.Solid.size(), 1);
		for (const FBox& Box : S.Course.Boxes)
		{
			if (IsCarryGround(S.Course, Box))
			{
				Cover(Box, 0.0, [&](int Cell) { Map.Solid[Cell] = 0; });
			}
		}
		Map.Wall = Map.Solid;
	}
	for (const FBox& Box : S.Course.Boxes)
	{
		if (IsCarryWall(S.Course, Box))
		{
			Cover(Box, 0.0, [&](int Cell) { Map.Solid[Cell] = 1; });
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

/**
 * Every plinth a machine lifted at plinth From can be walked to, given which switches are down.
 * A machine is set down and picked up at arm's length, so a plinth hard against a wall is served from the nearest
 * square a walker can stand in (three squares away at most, and never across a wall or a shut grille). A plinth
 * inside a shut grille is out of use while it is shut, as in the game.
 */
inline std::vector<bool> CarryReach(const FSetup& S, const FCarryMap& Map, int From, const std::vector<bool>& Held)
{
	// The state of the grilles decides everything, and there are few such states: each is worked out once.
	std::vector<bool> State(Map.Barred.size(), false);
	for (size_t G = 0; G < Map.Barred.size(); ++G)
	{
		State[G] = !Map.Barred[G].empty() && !Held[S.Course.Gates[G].Switch];
	}
	auto Found = Map.Groups.find(State);
	if (Found == Map.Groups.end())
	{
		std::vector<uint8_t> Bars(Map.Wall.size(), 0);
		for (size_t G = 0; G < Map.Barred.size(); ++G)
		{
			if (State[G])
			{
				for (const int Cell : Map.Barred[G])
				{
					Bars[Cell] = 1;
				}
			}
		}
		auto Around = [&](int Cell, int Next[4]) {
			const int X = Cell % Map.Width, Y = Cell / Map.Width;
			Next[0] = X > 0 ? Cell - 1 : -1;
			Next[1] = X + 1 < Map.Width ? Cell + 1 : -1;
			Next[2] = Y > 0 ? Cell - Map.Width : -1;
			Next[3] = Y + 1 < Map.Height ? Cell + Map.Width : -1;
		};
		// Where a walker stands to use each plinth: its own square if they can, or the nearest one they can.
		auto StandingCell = [&](int Plinth) {
			const int Own = Map.PlinthCell[Plinth];
			if (Own < 0 || Map.Solid[Own] || Bars[Own])
			{
				return -1;
			}
			std::vector<std::pair<int, int>> Open{{Own, 0}};
			std::vector<int> Tried{Own};
			for (size_t I = 0; I < Open.size(); ++I)
			{
				const int Cell = Open[I].first;
				if (!Map.Wall[Cell])
				{
					return Cell;
				}
				if (Open[I].second >= 3)
				{
					continue;
				}
				int Next[4];
				Around(Cell, Next);
				for (const int N : Next)
				{
					bool bNew = N >= 0 && !Map.Solid[N] && !Bars[N];
					for (size_t T = 0; T < Tried.size() && bNew; ++T)
					{
						bNew = Tried[T] != N;
					}
					if (bNew)
					{
						Tried.push_back(N);
						Open.push_back({N, Open[I].second + 1});
					}
				}
			}
			return -1;
		};
		// Flood out from each plinth not yet reached from another: every square gets the number of the first plinth
		// it can be walked to from.
		std::vector<int> Seen(Map.Wall.size(), -1);
		std::vector<int> Group(S.Plinths.size(), -1);
		for (size_t P = 0; P < S.Plinths.size(); ++P)
		{
			const int Start = StandingCell(static_cast<int>(P));
			if (Start < 0)
			{
				// Nobody can stand at it: out of use. (-1 is no group at all, not even its own.)
				continue;
			}
			if (Seen[Start] >= 0)
			{
				Group[P] = Seen[Start];
				continue;
			}
			Group[P] = static_cast<int>(P);
			std::queue<int> Open;
			Open.push(Start);
			Seen[Start] = Group[P];
			while (!Open.empty())
			{
				const int Cell = Open.front();
				Open.pop();
				int Next[4];
				Around(Cell, Next);
				for (const int N : Next)
				{
					if (N >= 0 && Seen[N] < 0 && !Map.Wall[N] && !Bars[N])
					{
						Seen[N] = Group[P];
						Open.push(N);
					}
				}
			}
		}
		Found = Map.Groups.emplace(State, std::move(Group)).first;
	}
	const std::vector<int>& Group = Found->second;
	std::vector<bool> Reached(S.Plinths.size(), false);
	if (From < 0 || From >= static_cast<int>(Group.size()) || Group[From] < 0)
	{
		return Reached;
	}
	for (size_t P = 0; P < S.Plinths.size(); ++P)
	{
		Reached[P] = Group[P] == Group[From];
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
	std::unordered_map<long long, std::pair<long long, FCarry>> Reached;
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
