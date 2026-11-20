// Design aid for holes whose puzzle is the order of carrying (grilles). It knows nothing of the ball or of
// geometry: a hole is rooms in a row, the walls between them, grilles in the walls, weight plates in the rooms, and
// machines that are heavy (hold a plate) or light (a mirror: hold nothing). It tries every such layout of a given
// size and reports the ones whose shortest answer is longest: the layouts where the pieces get in each other's way
// the most. A layout found here still has to be drawn as a real hole and proven (Tests/Sim/solve.sh).
//
//   rooms <rooms> <plates> <heavy> <light> [spares per room = 1] [side grilles per wall = 1] [show = 12]
//
// The rules are the game's. A grille is open while a heavy machine stands on its plate. A lifted machine holds
// nothing. A carried machine goes wherever open grilles lead, and is set down on any free plinth there: a plate, a
// spare plinth, a start. The swing needs every wall's ball grille open at once, and the light machine, if there is
// one, on the spare plinth of the last room (it turns the ball to the cup).

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <queue>
#include <string>
#include <vector>

struct FLayout
{
	std::vector<int> PlateRoom;          // room of each plate
	std::vector<int> Ball;               // per wall: the plate that opens its ball grille
	std::vector<std::vector<int>> Side;  // per wall: plates of its side grilles (-1: none)
};

struct FPlan
{
	int Length = -1;
	int States = 0;
	std::vector<std::string> Steps;
};

static int Rooms, Plates, Heavy, Light, Spares, Sides;

static FPlan Solve(const FLayout& L, bool bSteps)
{
	const int Machines = Heavy + Light;
	// Plinths: the starts (room 0), the plates, the spares.
	std::vector<int> Room;
	for (int M = 0; M < Machines; ++M) Room.push_back(0);
	const int FirstPlate = static_cast<int>(Room.size());
	for (int P = 0; P < Plates; ++P) Room.push_back(L.PlateRoom[P]);
	const int FirstSpare = static_cast<int>(Room.size());
	for (int R = 0; R < Rooms; ++R)
		for (int S = 0; S < Spares; ++S) Room.push_back(R);
	const int Plinths = static_cast<int>(Room.size());
	auto Key = [&](const std::vector<int>& On) {
		long long K = 0;
		for (const int P : On) K = K * Plinths + P;
		return K;
	};
	auto Held = [&](const std::vector<int>& On, int Lifted) {
		unsigned Mask = 0;
		for (int M = 0; M < Heavy; ++M)
			if (M != Lifted && On[M] >= FirstPlate && On[M] < FirstSpare) Mask |= 1u << (On[M] - FirstPlate);
		return Mask;
	};
	auto Done = [&](const std::vector<int>& On) {
		const unsigned Mask = Held(On, -1);
		for (int W = 0; W + 1 < Rooms; ++W)
			if (!(Mask >> L.Ball[W] & 1u)) return false;
		for (int M = Heavy; M < Machines; ++M)
			if (On[M] != FirstSpare + (Rooms - 1) * Spares) return false;
		return true;
	};
	std::vector<int> Start(Machines);
	for (int M = 0; M < Machines; ++M) Start[M] = M;
	std::map<long long, std::pair<long long, std::string>> Seen;
	std::queue<std::vector<int>> Open;
	Seen[Key(Start)] = {-1, ""};
	Open.push(Start);
	FPlan Plan;
	std::vector<int> End;
	while (!Open.empty())
	{
		const std::vector<int> On = Open.front();
		Open.pop();
		if (Done(On))
		{
			End = On;
			break;
		}
		for (int M = 0; M < Machines; ++M)
		{
			const unsigned Mask = Held(On, M);
			// The rooms a machine lifted here can be walked to.
			int Low = Room[On[M]], High = Room[On[M]];
			auto OpenWall = [&](int W) {
				if (Mask >> L.Ball[W] & 1u) return true;
				for (const int S : L.Side[W])
					if (S >= 0 && (Mask >> S & 1u)) return true;
				return false;
			};
			while (Low > 0 && OpenWall(Low - 1)) --Low;
			while (High + 1 < Rooms && OpenWall(High)) ++High;
			for (int P = 0; P < Plinths; ++P)
			{
				if (P == On[M] || Room[P] < Low || Room[P] > High) continue;
				// (Starts are places to stand like any other, but only the machine's own: it keeps the count down
				// and changes nothing, a spare plinth in the first room does the same job.)
				if (P < FirstPlate && P != M) continue;
				bool bFree = true;
				for (int O = 0; O < Machines && bFree; ++O) bFree = On[O] != P;
				if (!bFree) continue;
				std::vector<int> Next = On;
				Next[M] = P;
				const long long K = Key(Next);
				if (Seen.count(K)) continue;
				char Text[96];
				const char* Kind = M < Heavy ? "heavy" : "mirror";
				if (P >= FirstSpare)
					std::snprintf(Text, sizeof Text, "%s %d -> spare in room %d", Kind, M + 1, Room[P] + 1);
				else if (P >= FirstPlate)
					std::snprintf(Text, sizeof Text, "%s %d -> plate %c (room %d)", Kind, M + 1, 'A' + P - FirstPlate, Room[P] + 1);
				else
					std::snprintf(Text, sizeof Text, "%s %d -> its start", Kind, M + 1);
				Seen[K] = {Key(On), Text};
				Open.push(Next);
			}
		}
	}
	Plan.States = static_cast<int>(Seen.size());
	if (!End.empty())
	{
		Plan.Length = 0;
		for (long long K = Key(End); Seen[K].first != -1; K = Seen[K].first)
		{
			++Plan.Length;
			if (bSteps) Plan.Steps.insert(Plan.Steps.begin(), Seen[K].second);
		}
	}
	return Plan;
}

static std::string Describe(const FLayout& L)
{
	std::string Out;
	char Text[96];
	for (int P = 0; P < Plates; ++P)
	{
		std::snprintf(Text, sizeof Text, "plate %c in room %d; ", 'A' + P, L.PlateRoom[P] + 1);
		Out += Text;
	}
	for (int W = 0; W + 1 < Rooms; ++W)
	{
		std::snprintf(Text, sizeof Text, "wall %d: ball grille %c", W + 1, 'A' + L.Ball[W]);
		Out += Text;
		for (const int S : L.Side[W])
			if (S >= 0)
			{
				std::snprintf(Text, sizeof Text, ", side grille %c", 'A' + S);
				Out += Text;
			}
		Out += "; ";
	}
	return Out;
}

int main(int Argc, char** Argv)
{
	if (Argc < 5)
	{
		std::fprintf(stderr, "rooms <rooms> <plates> <heavy> <light> [spares per room] [side grilles per wall] [show]\n");
		return 2;
	}
	Rooms = std::atoi(Argv[1]);
	Plates = std::atoi(Argv[2]);
	Heavy = std::atoi(Argv[3]);
	Light = std::atoi(Argv[4]);
	Spares = Argc > 5 ? std::atoi(Argv[5]) : 1;
	Sides = Argc > 6 ? std::atoi(Argv[6]) : 1;
	const int Show = Argc > 7 ? std::atoi(Argv[7]) : 12;
	const int Walls = Rooms - 1;

	struct FFound
	{
		int Length;
		int Parts;
		FLayout Layout;
	};
	std::vector<FFound> Found;
	std::map<int, int> Histogram;
	long long Tried = 0;
	FLayout L;
	L.PlateRoom.assign(Plates, 0);
	L.Ball.assign(Walls, 0);
	L.Side.assign(Walls, std::vector<int>(Sides, -1));
	// Odometers over every choice.
	std::vector<int*> Dials;
	std::vector<int> Lows, Highs;
	for (int P = 0; P < Plates; ++P) { Dials.push_back(&L.PlateRoom[P]); Lows.push_back(0); Highs.push_back(Rooms - 1); }
	for (int W = 0; W < Walls; ++W) { Dials.push_back(&L.Ball[W]); Lows.push_back(0); Highs.push_back(Plates - 1); }
	for (int W = 0; W < Walls; ++W)
		for (int S = 0; S < Sides; ++S) { Dials.push_back(&L.Side[W][S]); Lows.push_back(-1); Highs.push_back(Plates - 1); }
	for (size_t D = 0; D < Dials.size(); ++D) *Dials[D] = Lows[D];
	for (;;)
	{
		// Skip layouts that are another one renamed or padded: plates in room order, every plate used by some
		// grille, side grilles of a wall in order and none repeating the ball grille's plate.
		bool bPlain = true;
		for (int P = 0; P + 1 < Plates && bPlain; ++P) bPlain = L.PlateRoom[P] <= L.PlateRoom[P + 1];
		std::vector<int> Used(Plates, 0);
		for (int W = 0; W < Walls && bPlain; ++W)
		{
			Used[L.Ball[W]] = 1;
			for (int S = 0; S < Sides && bPlain; ++S)
			{
				if (L.Side[W][S] >= 0) Used[L.Side[W][S]] = 1;
				bPlain = L.Side[W][S] != L.Ball[W] && (S == 0 || L.Side[W][S] == -1 || L.Side[W][S] > L.Side[W][S - 1]);
			}
		}
		for (int P = 0; P < Plates && bPlain; ++P) bPlain = Used[P] != 0;
		// The swing needs every ball grille open at once: no more ball plates than heavy machines.
		std::vector<int> BallPlates = L.Ball;
		std::sort(BallPlates.begin(), BallPlates.end());
		BallPlates.erase(std::unique(BallPlates.begin(), BallPlates.end()), BallPlates.end());
		if (bPlain && static_cast<int>(BallPlates.size()) <= Heavy)
		{
			++Tried;
			const FPlan Plan = Solve(L, false);
			++Histogram[Plan.Length];
			if (Plan.Length > 0)
			{
				int Parts = Plates;
				for (int W = 0; W < Walls; ++W)
					for (const int S : L.Side[W]) Parts += S >= 0;
				Found.push_back({Plan.Length, Parts, L});
			}
		}
		size_t D = 0;
		while (D < Dials.size() && ++*Dials[D] > Highs[D]) { *Dials[D] = Lows[D]; ++D; }
		if (D == Dials.size()) break;
	}
	std::sort(Found.begin(), Found.end(), [](const FFound& A, const FFound& B) { return A.Length != B.Length ? A.Length > B.Length : A.Parts < B.Parts; });
	std::printf("%lld layouts of %d rooms, %d plates, %d heavy, %d light, %d spare a room, %d side grilles a wall\n", Tried, Rooms, Plates, Heavy, Light, Spares, Sides);
	for (const auto& [Length, Count] : Histogram)
		std::printf("  %s: %d\n", Length < 0 ? "cannot be done" : (std::to_string(Length) + " carries").c_str(), Count);
	for (int I = 0; I < Show && I < static_cast<int>(Found.size()); ++I)
	{
		const FPlan Plan = Solve(Found[I].Layout, true);
		std::printf("\n%d carries, %d ways of standing reached: %s\n", Plan.Length, Plan.States, Describe(Found[I].Layout).c_str());
		for (const std::string& Step : Plan.Steps) std::printf("    %s\n", Step.c_str());
	}
	return 0;
}
