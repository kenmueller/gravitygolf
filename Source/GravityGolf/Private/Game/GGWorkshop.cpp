#include "Game/GGWorkshop.h"

#include "Async/ParallelFor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Game/GGGolfer.h"
#include "Game/GGHUD.h"
#include "Game/GGRival.h"
#include "Game/GGSave.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <atomic>

DEFINE_LOG_CATEGORY_STATIC(LogGGWorkshop, Log, All);

namespace
{
// How a design becomes a course. These match the game's own holes (Tools/courses).
constexpr double WallHeight = 150.0;
constexpr double CupRadius = 42.0;
constexpr double CupCapture = 650.0;
constexpr double CourtRadius = 420.0;
constexpr double PadRadius = 240.0;
constexpr double LightPlateRadius = 70.0;
constexpr double WeightPlateRadius = 100.0;
constexpr double PullStrength = 420.0;
constexpr double PullReach = 1900.0;
constexpr double PushStrength = -420.0;
constexpr double PushReach = 1500.0;
constexpr double MachineCore = 45.0;
constexpr int32 MaxSquares = 16000;
constexpr int32 MaxExtent = 400; // squares from the origin, each way: 200 m
constexpr int32 ProofRows = 720;
constexpr int32 ProofPowers = 77;
// The rating: this many random placements a swing, each tried with 180 directions and 20 strengths.
constexpr int32 RatePlacements = 120;
constexpr int32 RateRows = 180;
constexpr int32 RatePowers = 20;

TArray<TSharedPtr<FJsonValue>> Nums(std::initializer_list<double> Values)
{
	TArray<TSharedPtr<FJsonValue>> Out;
	for (const double Value : Values)
	{
		Out.Add(MakeShared<FJsonValueNumber>(Value));
	}
	return Out;
}

bool ReadNumbers(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32 Count, TArray<double>& Out)
{
	Out.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || Values->Num() < Count)
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Values)
	{
		Out.Add(Value->AsNumber());
	}
	return true;
}

FIntPoint SpotOf(double X, double Y)
{
	// The middle of the square the point is in.
	return FIntPoint(FMath::FloorToInt32(X / FGGDesign::Cell) * FGGDesign::Cell + FGGDesign::Cell / 2,
		FMath::FloorToInt32(Y / FGGDesign::Cell) * FGGDesign::Cell + FGGDesign::Cell / 2);
}

FIntRect RectOf(const TArray<double>& Center, const TArray<double>& Size)
{
	return FIntRect(FMath::RoundToInt32(Center[0] - Size[0] * 0.5), FMath::RoundToInt32(Center[1] - Size[1] * 0.5),
		FMath::RoundToInt32(Center[0] + Size[0] * 0.5), FMath::RoundToInt32(Center[1] + Size[1] * 0.5));
}
} // namespace

// ------------------------------------------------------------------------------------------------ the design

FGGDesign FGGDesign::Starter()
{
	// A strip of ground, a wall across the straight line, and one attractor waiting by the tee: already a small
	// puzzle, and an example of what the tools make.
	FGGDesign Design;
	Design.Paint(FIntRect(0, -10, 56, 10), EGGCell::Floor);
	Design.Paint(FIntRect(26, -5, 28, 5), EGGCell::Soft);
	Design.Tee = FIntPoint(325, 25);
	Design.Cup = FIntPoint(2425, 25);
	Design.Machines.Add({FIntPoint(325, -275), false});
	return Design;
}

void FGGDesign::Paint(const FIntRect& Squares, EGGCell Kind)
{
	for (int32 Y = FMath::Max(Squares.Min.Y, -MaxExtent); Y < FMath::Min(Squares.Max.Y, MaxExtent); ++Y)
	{
		for (int32 X = FMath::Max(Squares.Min.X, -MaxExtent); X < FMath::Min(Squares.Max.X, MaxExtent); ++X)
		{
			if (Kind == EGGCell::None)
			{
				Cells.Remove(FIntPoint(X, Y));
			}
			else if (Cells.Num() < MaxSquares || Cells.Contains(FIntPoint(X, Y)))
			{
				Cells.Add(FIntPoint(X, Y), Kind);
			}
		}
	}
}

EGGCell FGGDesign::AtPoint(const FIntPoint& Point) const
{
	return At(FIntPoint(FMath::FloorToInt32(static_cast<double>(Point.X) / Cell), FMath::FloorToInt32(static_cast<double>(Point.Y) / Cell)));
}

TArray<FIntRect> FGGDesign::Rects(EGGCell Kind) const
{
	// Greedy: take the first square left (in reading order), stretch it right, then down. The order is fixed, so
	// every machine cuts the same design into the same boxes, and the simulation meets them in the same order.
	TSet<FIntPoint> Left;
	for (const TPair<FIntPoint, EGGCell>& Pair : Cells)
	{
		// The floor is every square with ground except the sand, which is ground of its own kind: two boxes sharing
		// the ground under the ball would leave only the first one touching it.
		if (Kind == EGGCell::Floor ? (Pair.Value != EGGCell::None && Pair.Value != EGGCell::Sand) : Pair.Value == Kind)
		{
			Left.Add(Pair.Key);
		}
	}
	TArray<FIntPoint> Order = Left.Array();
	Order.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y != B.Y ? A.Y < B.Y : A.X < B.X; });
	TArray<FIntRect> Out;
	for (const FIntPoint& Start : Order)
	{
		if (!Left.Contains(Start))
		{
			continue;
		}
		int32 Width = 1;
		while (Left.Contains(FIntPoint(Start.X + Width, Start.Y)))
		{
			++Width;
		}
		int32 Height = 1;
		for (;;)
		{
			bool bRow = true;
			for (int32 X = 0; X < Width && bRow; ++X)
			{
				bRow = Left.Contains(FIntPoint(Start.X + X, Start.Y + Height));
			}
			if (!bRow)
			{
				break;
			}
			++Height;
		}
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				Left.Remove(FIntPoint(Start.X + X, Start.Y + Y));
			}
		}
		Out.Add(FIntRect(Start.X * Cell, Start.Y * Cell, (Start.X + Width) * Cell, (Start.Y + Height) * Cell));
	}
	return Out;
}

bool FGGDesign::Bounds(FIntRect& Out) const
{
	bool bAny = false;
	for (const TPair<FIntPoint, EGGCell>& Pair : Cells)
	{
		const FIntRect Square(Pair.Key.X * Cell, Pair.Key.Y * Cell, (Pair.Key.X + 1) * Cell, (Pair.Key.Y + 1) * Cell);
		if (bAny)
		{
			Out.Union(Square);
		}
		else
		{
			Out = Square;
		}
		bAny = true;
	}
	return bAny;
}

const TCHAR* FGGDesign::RatingName(int32 Luck)
{
	// The solver's own line for the game's holes is 35 in 100: above it, where the machine goes barely matters.
	return Luck < 0 ? TEXT("") : Luck > 35 ? TEXT("Too easy") : Luck > 15 ? TEXT("Easy") : Luck > 5 ? TEXT("Medium") : Luck > 1 ? TEXT("Hard") : TEXT("Fiendish");
}

bool FGGDesign::Solved() const
{
	for (int32 Leg = 0; Leg < Legs(); ++Leg)
	{
		if (!Solutions.ContainsByPredicate([&](const FGGSolution& Answer) { return Answer.Leg == Leg; }))
		{
			return false;
		}
	}
	return true;
}

FString FGGDesign::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("id"), TEXT("workshop"));
	Root->SetStringField(TEXT("name"), Name);
	Root->SetStringField(TEXT("level"), TEXT("Workshop"));
	Root->SetNumberField(TEXT("par"), Legs());
	Root->SetNumberField(TEXT("ballRadius"), 6);
	Root->SetArrayField(TEXT("gravity"), Nums({0, 0, -980}));
	// The ground is a slab 30 cm thick lying on the Workshop's table: a ball that rolls off its edge is lost at once.
	Root->SetNumberField(TEXT("killZ"), -22);
	Root->SetBoolField(TEXT("drawn"), true);
	Root->SetStringField(TEXT("ambience"), TEXT("hum"));

	const TSharedRef<FJsonObject> TeeObject = MakeShared<FJsonObject>();
	TeeObject->SetArrayField(TEXT("position"), Nums({double(Tee.X), double(Tee.Y), 6}));
	TeeObject->SetNumberField(TEXT("yaw"), FMath::RoundToDouble(FMath::RadiansToDegrees(FMath::Atan2(double(Cup.Y - Tee.Y), double(Cup.X - Tee.X)))));
	Root->SetObjectField(TEXT("tee"), TeeObject);
	const TSharedRef<FJsonObject> CupObject = MakeShared<FJsonObject>();
	CupObject->SetArrayField(TEXT("position"), Nums({double(Cup.X), double(Cup.Y), 0}));
	CupObject->SetNumberField(TEXT("radius"), CupRadius);
	CupObject->SetNumberField(TEXT("captureSpeed"), CupCapture);
	Root->SetObjectField(TEXT("cup"), CupObject);

	TArray<TSharedPtr<FJsonValue>> NoMachine;
	auto Keep = [&](const FIntPoint& At, double Radius, const TCHAR* Kind) {
		const TSharedRef<FJsonObject> Zone = MakeShared<FJsonObject>();
		Zone->SetArrayField(TEXT("position"), Nums({double(At.X), double(At.Y), 0}));
		Zone->SetNumberField(TEXT("radius"), Radius);
		Zone->SetStringField(TEXT("kind"), Kind);
		NoMachine.Add(MakeShared<FJsonValueObject>(Zone));
	};
	Keep(Cup, CourtRadius, TEXT("court"));

	TArray<TSharedPtr<FJsonValue>> Boxes;
	auto Box = [&](const FString& BoxName, double CX, double CY, double CZ, double SX, double SY, double SZ, const TCHAR* Surface, const TCHAR* Look) {
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("name"), BoxName);
		Object->SetArrayField(TEXT("center"), Nums({CX, CY, CZ}));
		Object->SetArrayField(TEXT("size"), Nums({SX, SY, SZ}));
		Object->SetStringField(TEXT("surface"), Surface);
		Object->SetStringField(TEXT("look"), Look);
		Boxes.Add(MakeShared<FJsonValueObject>(Object));
	};
	auto Layer = [&](EGGCell Kind, const TCHAR* Prefix, double CZ, double SZ, const TCHAR* Surface, const TCHAR* Look) {
		const TArray<FIntRect> All = Rects(Kind);
		for (int32 I = 0; I < All.Num(); ++I)
		{
			const FIntRect& R = All[I];
			Box(FString::Printf(TEXT("%s%d"), Prefix, I + 1), (R.Min.X + R.Max.X) * 0.5, (R.Min.Y + R.Max.Y) * 0.5, CZ, R.Width(), R.Height(), SZ,
				Surface, Look);
		}
	};
	Layer(EGGCell::Floor, TEXT("Floor"), -15, 30, TEXT("green"), TEXT("floor"));
	Layer(EGGCell::Sand, TEXT("Sand"), -15, 30, TEXT("sand"), TEXT("sand"));

	TArray<TSharedPtr<FJsonValue>> Zones;
	for (int32 I = 0; I < Pads.Num(); ++I)
	{
		const TSharedRef<FJsonObject> Zone = MakeShared<FJsonObject>();
		Zone->SetArrayField(TEXT("position"), Nums({double(Pads[I].X), double(Pads[I].Y), 0}));
		Zone->SetNumberField(TEXT("radius"), PadRadius);
		Zone->SetNumberField(TEXT("needsSwitch"), -1);
		Zones.Add(MakeShared<FJsonValueObject>(Zone));
		Keep(Pads[I], PadRadius + 60, TEXT("pad"));
	}
	Root->SetArrayField(TEXT("safeZones"), Zones);
	Layer(EGGCell::Wall, TEXT("Wall"), WallHeight * 0.5, WallHeight, TEXT("stone"), TEXT("wall"));
	Layer(EGGCell::Soft, TEXT("Soft"), WallHeight * 0.5, WallHeight, TEXT("sand"), TEXT("soft"));
	Root->SetArrayField(TEXT("boxes"), Boxes);

	TArray<TSharedPtr<FJsonValue>> Switches;
	for (const FPlate& Plate : Plates)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetArrayField(TEXT("position"), Nums({double(Plate.At.X), double(Plate.At.Y), 0}));
		Object->SetNumberField(TEXT("radius"), Plate.bWeight ? WeightPlateRadius : LightPlateRadius);
		Object->SetBoolField(TEXT("weight"), Plate.bWeight);
		Switches.Add(MakeShared<FJsonValueObject>(Object));
		if (!Plate.bWeight)
		{
			Keep(Plate.At, LightPlateRadius + 40, TEXT("switch"));
		}
	}
	Root->SetArrayField(TEXT("switches"), Switches);
	Root->SetArrayField(TEXT("noMachine"), NoMachine);

	TArray<TSharedPtr<FJsonValue>> GateValues;
	for (int32 I = 0; I < Gates.Num(); ++I)
	{
		const FIntRect& R = Gates[I].Rect;
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("name"), FString::Printf(TEXT("Gate%d"), I + 1));
		Object->SetArrayField(TEXT("center"), Nums({(R.Min.X + R.Max.X) * 0.5, (R.Min.Y + R.Max.Y) * 0.5, WallHeight * 0.5}));
		Object->SetArrayField(TEXT("size"), Nums({double(R.Width()), double(R.Height()), WallHeight}));
		Object->SetNumberField(TEXT("switch"), Gates[I].Plate);
		Object->SetNumberField(TEXT("openSeconds"), 0);
		Object->SetStringField(TEXT("surface"), TEXT("sand"));
		GateValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("gates"), GateValues);

	TArray<TSharedPtr<FJsonValue>> MachineValues;
	for (const FMachine& Machine : Machines)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetArrayField(TEXT("position"), Nums({double(Machine.At.X), double(Machine.At.Y), 0}));
		Object->SetNumberField(TEXT("strength"), Machine.bMirror ? 0.0 : Machine.bPush ? PushStrength : PullStrength);
		Object->SetNumberField(TEXT("range"), Machine.bMirror ? 0.0 : Machine.bPush ? PushReach : PullReach);
		if (Machine.bMirror)
		{
			Object->SetStringField(TEXT("kind"), TEXT("mirror"));
			Object->SetNumberField(TEXT("turn"), Machine.Turn);
		}
		Object->SetNumberField(TEXT("coreHeight"), MachineCore);
		Object->SetBoolField(TEXT("movable"), true);
		MachineValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("attractors"), MachineValues);

	TArray<TSharedPtr<FJsonValue>> Answers;
	for (const FGGSolution& Answer : Solutions)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("leg"), Answer.Leg);
		Object->SetNumberField(TEXT("yaw"), Answer.Yaw);
		Object->SetNumberField(TEXT("power"), Answer.Power);
		TArray<TSharedPtr<FJsonValue>> Spots;
		for (const FVector& Spot : Answer.Machines)
		{
			Spots.Add(MakeShared<FJsonValueArray>(Nums({Spot.X, Spot.Y})));
		}
		Object->SetArrayField(TEXT("machines"), Spots);
		TArray<TSharedPtr<FJsonValue>> HeldValues;
		for (const bool bHeld : Answer.Held)
		{
			HeldValues.Add(MakeShared<FJsonValueNumber>(bHeld ? 1 : 0));
		}
		Object->SetArrayField(TEXT("held"), HeldValues);
		TArray<TSharedPtr<FJsonValue>> TurnValues;
		for (const int32 Turn : Answer.Turns)
		{
			TurnValues.Add(MakeShared<FJsonValueNumber>(Turn));
		}
		Object->SetArrayField(TEXT("turns"), TurnValues);
		Answers.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("solutions"), Answers);
	Root->SetNumberField(TEXT("necessary"), Necessary);
	Root->SetNumberField(TEXT("luck"), Luck);

	FString Text;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
	FJsonSerializer::Serialize(Root, Writer);
	return Text;
}

bool FGGDesign::FromJson(const FString& Text)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		return false;
	}
	FGGDesign Design;
	Root->TryGetStringField(TEXT("name"), Design.Name);
	Design.Name = Design.Name.Left(28);
	TArray<double> A;
	TArray<double> B;
	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Root->TryGetObjectField(TEXT("tee"), Object) || !ReadNumbers(*Object, TEXT("position"), 2, A))
	{
		return false;
	}
	Design.Tee = SpotOf(A[0], A[1]);
	if (!Root->TryGetObjectField(TEXT("cup"), Object) || !ReadNumbers(*Object, TEXT("position"), 2, A))
	{
		return false;
	}
	Design.Cup = SpotOf(A[0], A[1]);

	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (Root->TryGetArrayField(TEXT("boxes"), Values))
	{
		// Ground first, then what stands on it.
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				const TSharedPtr<FJsonObject> Box = Value->AsObject();
				FString Look;
				if (!Box.IsValid() || !Box->TryGetStringField(TEXT("look"), Look) || !ReadNumbers(Box, TEXT("center"), 3, A) || !ReadNumbers(Box, TEXT("size"), 3, B))
				{
					continue;
				}
				const EGGCell Kind = Look == TEXT("floor") ? EGGCell::Floor : Look == TEXT("sand") ? EGGCell::Sand
					: Look == TEXT("wall") ? EGGCell::Wall : Look == TEXT("soft") ? EGGCell::Soft : EGGCell::None;
				if (Kind == EGGCell::None || (Kind == EGGCell::Floor) != (Pass == 0))
				{
					continue;
				}
				const FIntRect R = RectOf(A, B);
				Design.Paint(FIntRect(FMath::RoundToInt32(double(R.Min.X) / Cell), FMath::RoundToInt32(double(R.Min.Y) / Cell),
					FMath::RoundToInt32(double(R.Max.X) / Cell), FMath::RoundToInt32(double(R.Max.Y) / Cell)), Kind);
			}
		}
	}
	if (Root->TryGetArrayField(TEXT("safeZones"), Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (ReadNumbers(Value->AsObject(), TEXT("position"), 2, A) && Design.Pads.Num() < 3)
			{
				Design.Pads.Add(SpotOf(A[0], A[1]));
			}
		}
	}
	if (Root->TryGetArrayField(TEXT("switches"), Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (ReadNumbers(Value->AsObject(), TEXT("position"), 2, A) && Design.Plates.Num() < 6)
			{
				FPlate Plate;
				Plate.At = SpotOf(A[0], A[1]);
				Value->AsObject()->TryGetBoolField(TEXT("weight"), Plate.bWeight);
				Design.Plates.Add(Plate);
			}
		}
	}
	if (Root->TryGetArrayField(TEXT("gates"), Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			int32 Plate = 0;
			if (ReadNumbers(Value->AsObject(), TEXT("center"), 3, A) && ReadNumbers(Value->AsObject(), TEXT("size"), 3, B)
				&& Value->AsObject()->TryGetNumberField(TEXT("switch"), Plate) && Design.Plates.IsValidIndex(Plate) && Design.Gates.Num() < 8)
			{
				Design.Gates.Add({RectOf(A, B), Plate});
			}
		}
	}
	if (Root->TryGetArrayField(TEXT("attractors"), Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			double Strength = 0.0;
			if (ReadNumbers(Value->AsObject(), TEXT("position"), 2, A) && Value->AsObject()->TryGetNumberField(TEXT("strength"), Strength)
				&& Design.Machines.Num() < 4)
			{
				FMachine Machine{SpotOf(A[0], A[1]), Strength < 0.0};
				FString Kind;
				if (Value->AsObject()->TryGetStringField(TEXT("kind"), Kind) && Kind == TEXT("mirror"))
				{
					Machine.bPush = false;
					Machine.bMirror = true;
					Value->AsObject()->TryGetNumberField(TEXT("turn"), Machine.Turn);
					Machine.Turn = Machine.Turn ? 1 : 0;
				}
				Design.Machines.Add(Machine);
			}
		}
	}
	if (Root->TryGetArrayField(TEXT("solutions"), Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const TSharedPtr<FJsonObject> Answer = Value->AsObject();
			FGGSolution Solution;
			const TArray<TSharedPtr<FJsonValue>>* Spots = nullptr;
			if (!Answer.IsValid() || !Answer->TryGetNumberField(TEXT("leg"), Solution.Leg) || !Answer->TryGetNumberField(TEXT("yaw"), Solution.Yaw)
				|| !Answer->TryGetNumberField(TEXT("power"), Solution.Power) || !Answer->TryGetArrayField(TEXT("machines"), Spots)
				|| Spots->Num() != Design.Machines.Num())
			{
				continue;
			}
			bool bGood = true;
			for (const TSharedPtr<FJsonValue>& Spot : *Spots)
			{
				const TArray<TSharedPtr<FJsonValue>>* XY = nullptr;
				if (Spot->TryGetArray(XY) && XY->Num() >= 2)
				{
					Solution.Machines.Add(FVector((*XY)[0]->AsNumber(), (*XY)[1]->AsNumber(), 0.0));
				}
				else
				{
					bGood = false;
				}
			}
			if (ReadNumbers(Answer, TEXT("held"), 0, A))
			{
				for (const double Held : A)
				{
					Solution.Held.Add(Held != 0.0);
				}
			}
			if (ReadNumbers(Answer, TEXT("turns"), 0, A))
			{
				for (const double Turn : A)
				{
					Solution.Turns.Add(Turn != 0.0 ? 1 : 0);
				}
			}
			if (bGood)
			{
				Design.Solutions.Add(Solution);
			}
		}
	}
	Root->TryGetNumberField(TEXT("necessary"), Design.Necessary);
	Root->TryGetNumberField(TEXT("luck"), Design.Luck);
	*this = MoveTemp(Design);
	return true;
}

// ------------------------------------------------------------------------------------------------ the workshop

AGGWorkshop::AGGWorkshop()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(50.f);
}

AGGWorkshop* AGGWorkshop::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (World)
	{
		if (TActorIterator<AGGWorkshop> It(World); It)
		{
			return *It;
		}
	}
	return nullptr;
}

APlayerController* AGGWorkshop::Player() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}

AGGGolfer* AGGWorkshop::Golfer() const
{
	const APlayerController* PC = Player();
	return PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
}

const TCHAR* AGGWorkshop::ToolName(EGGTool Tool)
{
	static const TCHAR* Names[] = {TEXT("Move"), TEXT("Ground"), TEXT("Sand"), TEXT("Wall"), TEXT("Soft wall"), TEXT("Safe pad"), TEXT("Light plate"),
		TEXT("Weight plate"), TEXT("Gate"), TEXT("Attractor"), TEXT("Repulsor"), TEXT("Mirror"), TEXT("Erase")};
	return Names[FMath::Clamp(static_cast<int32>(Tool), 0, static_cast<int32>(EGGTool::Count) - 1)];
}

const TCHAR* AGGWorkshop::ToolHelp(EGGTool Tool)
{
	static const TCHAR* Help[] = {
		TEXT("Drag the tee, the cup, a machine, a plate, a pad or a gate to move it"),
		TEXT("Drag to lay ground. A ball that rolls off its edge is lost"),
		TEXT("Drag to lay sand. It slows the ball"),
		TEXT("Drag to raise a stone wall. The ball bounces off it"),
		TEXT("Drag to raise a soft wall. The ball dies against it, so nothing banks"),
		TEXT("A ball that rests on a safe pad stays there: the hole becomes a chain of swings"),
		TEXT("The ball opens this plate's gates by rolling over it"),
		TEXT("This plate's gates are open only while a machine stands on it"),
		TEXT("Drag to bar a gap. It opens with the plate placed last; click a gate to give it another plate"),
		TEXT("Bends the ball toward it as the ball comes, then lets it go"),
		TEXT("Pushes the ball away"),
		TEXT("The ball bounces off it at a right angle. Click a mirror to turn it to its other diagonal"),
		TEXT("Click a thing to remove it, or drag across ground to clear it"),
	};
	return Help[FMath::Clamp(static_cast<int32>(Tool), 0, static_cast<int32>(EGGTool::Count) - 1)];
}

void AGGWorkshop::Say(const FString& Text)
{
	Note = Text;
	NoteTime = 6.0;
	UE_LOG(LogGGWorkshop, Display, TEXT("Workshop: %s"), *Text);
}

void AGGWorkshop::Begin(AGGCourse* InCourse)
{
	Hole = InCourse;
	const UGGSave& Save = UGGSave::Get();
	if (Save.Draft.IsEmpty() || !Current.FromJson(Save.Draft))
	{
		Current = FGGDesign::Starter();
	}
	Rebuild();
	FrameDesign();
	StartBuilding();
}

void AGGWorkshop::Rebuild()
{
	if (Hole)
	{
		if (AGGGolfer* Pawn = Golfer())
		{
			// The machines are about to be rebuilt: nobody is carrying one any more.
			Pawn->SetCarried(INDEX_NONE);
		}
		Hole->LoadText(Current.ToJson(), TEXT("workshop"));
		Hole->HideFields(bBuilding);
		SeenPlayed = Hole->PlayedSerial();
	}
}

void AGGWorkshop::Changed(const FString& Before)
{
	History.Add(Before);
	if (History.Num() > 80)
	{
		History.RemoveAt(0);
	}
	// A changed hole is a new hole: whatever was proven about the old one no longer counts.
	Current.Solutions.Reset();
	Current.Necessary = 0;
	Current.Luck = -1;
	bProving = false;
	bRating = false;
	Rebuild();
	UGGSave::Get().Draft = Current.ToJson();
	bDraftDirty = true;
}

void AGGWorkshop::Undo()
{
	if (History.Num() > 0 && bBuilding)
	{
		Cancel();
		Current.FromJson(History.Pop());
		bProving = false;
		bRating = false;
		Rebuild();
		UGGSave::Get().Draft = Current.ToJson();
		bDraftDirty = true;
	}
}

void AGGWorkshop::SetTool(EGGTool Tool)
{
	Cancel();
	CurrentTool = Tool;
	Say(ToolHelp(Tool));
}

// ---- the view

void AGGWorkshop::PlaceCamera()
{
	Distance = FMath::Clamp(Distance, 700.0, 14000.0);
	// Looking down and along -Y, so +X runs to the right: holes are usually longest from tee to cup, and screens are wide.
	const FRotator Look(-64.0, -90.0, 0.0);
	Camera->SetWorldLocationAndRotation(FVector(Focus.X, Focus.Y, 0.0) - Look.Vector() * Distance, Look);
}

void AGGWorkshop::LookAt(const FVector2D& World, double InDistance)
{
	Focus = World;
	Distance = InDistance;
	PlaceCamera();
}

void AGGWorkshop::FrameDesign()
{
	FIntRect Bounds;
	if (Current.Bounds(Bounds))
	{
		// Far enough back for the hole's length across the screen and its width up it, and a little low, to leave room
		// for the tools along the bottom.
		const double Back = FMath::Max(Bounds.Width() / 0.74, Bounds.Height() / 0.4);
		LookAt(FVector2D((Bounds.Min.X + Bounds.Max.X) * 0.5, (Bounds.Min.Y + Bounds.Max.Y) * 0.5 + Back * 0.035), Back);
	}
}

bool AGGWorkshop::WorldAt(const FVector2D& Screen, FVector2D& OutWorld) const
{
	FVector Origin;
	FVector Direction;
	const APlayerController* PC = Player();
	if (PC && UGameplayStatics::DeprojectScreenToWorld(PC, Screen, Origin, Direction) && Direction.Z < -0.01)
	{
		OutWorld = FVector2D(Origin + Direction * (-Origin.Z / Direction.Z));
		return true;
	}
	return false;
}

// ---- building and testing

void AGGWorkshop::StartBuilding()
{
	bBuilding = true;
	Cancel();
	for (TActorIterator<AGGRival> Rival(GetWorld()); Rival; ++Rival)
	{
		Rival->Destroy();
	}
	// Put the hole back as designed (a test may have moved the machines and the ball).
	Rebuild();
	if (AGGGolfer* Pawn = Golfer())
	{
		Pawn->SetActorHiddenInGame(true);
		Pawn->GetCharacterMovement()->DisableMovement();
	}
	if (APlayerController* PC = Player())
	{
		PC->SetViewTargetWithBlend(this, 0.35f);
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

bool AGGWorkshop::StartTesting()
{
	auto OnGround = [&](const FIntPoint& At) {
		const EGGCell Kind = Current.AtPoint(At);
		return Kind == EGGCell::Floor || Kind == EGGCell::Sand;
	};
	if (!OnGround(Current.Tee))
	{
		Say(TEXT("The tee has to stand on open ground. Move it, or lay ground under it."));
		return false;
	}
	if (!OnGround(Current.Cup))
	{
		Say(TEXT("The cup has to be in open ground. Move it, or lay ground under it."));
		return false;
	}
	Cancel();
	bBuilding = false;
	bProving = false;
	bRating = false;
	Rebuild();
	// A rival, if the player races one, can race a hole of their own too, once it has an answer for it to follow.
	if (UGGSave::Get().Rival > 0 && Current.Solved() && GetNetMode() == NM_Standalone)
	{
		GetWorld()->SpawnActor<AGGRival>()->Begin(Hole, UGGSave::Get().Rival);
	}
	AGGGolfer* Pawn = Golfer();
	APlayerController* PC = Player();
	if (Pawn && PC)
	{
		Pawn->SetActorHiddenInGame(false);
		Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Pawn->GoToBall();
		PC->SetViewTargetWithBlend(Pawn, 0.35f);
		if (const AGGHUD* Hud = Cast<AGGHUD>(PC->GetHUD()); !Hud || !Hud->MenuOpen())
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
	Note.Reset();
	UGGSave::Get().Draft = Current.ToJson();
	UGGSave::Store();
	bDraftDirty = false;
	return true;
}

void AGGWorkshop::Toggle()
{
	if (bBuilding)
	{
		StartTesting();
	}
	else
	{
		StartBuilding();
	}
}

// ---- the tools

FIntPoint AGGWorkshop::Square(const FVector2D& World)
{
	return FIntPoint(FMath::FloorToInt32(World.X / FGGDesign::Cell), FMath::FloorToInt32(World.Y / FGGDesign::Cell));
}

FIntPoint AGGWorkshop::Spot(const FVector2D& World)
{
	return SpotOf(World.X, World.Y);
}

AGGWorkshop::EPick AGGWorkshop::PickAt(const FVector2D& World, int32& OutIndex) const
{
	auto Near = [&](const FIntPoint& At, double Reach) { return FVector2D::DistSquared(World, FVector2D(At.X, At.Y)) <= Reach * Reach; };
	OutIndex = INDEX_NONE;
	for (int32 I = Current.Machines.Num() - 1; I >= 0; --I)
	{
		if (Near(Current.Machines[I].At, 70.0))
		{
			OutIndex = I;
			return EPick::Machine;
		}
	}
	if (Near(Current.Tee, 80.0))
	{
		return EPick::Tee;
	}
	if (Near(Current.Cup, 70.0))
	{
		return EPick::Cup;
	}
	for (int32 I = Current.Plates.Num() - 1; I >= 0; --I)
	{
		if (Near(Current.Plates[I].At, 100.0))
		{
			OutIndex = I;
			return EPick::Plate;
		}
	}
	for (int32 I = Current.Gates.Num() - 1; I >= 0; --I)
	{
		const FIntRect& R = Current.Gates[I].Rect;
		if (World.X >= R.Min.X && World.X < R.Max.X && World.Y >= R.Min.Y && World.Y < R.Max.Y)
		{
			OutIndex = I;
			return EPick::Gate;
		}
	}
	for (int32 I = Current.Pads.Num() - 1; I >= 0; --I)
	{
		if (Near(Current.Pads[I], PadRadius))
		{
			OutIndex = I;
			return EPick::Pad;
		}
	}
	return EPick::None;
}

void AGGWorkshop::RemovePlate(int32 Index)
{
	// Its gates go with it; the gates of later plates follow their plates down a number.
	Current.Gates.RemoveAll([&](const FGGDesign::FGate& Gate) { return Gate.Plate == Index; });
	for (FGGDesign::FGate& Gate : Current.Gates)
	{
		if (Gate.Plate > Index)
		{
			--Gate.Plate;
		}
	}
	Current.Plates.RemoveAt(Index);
}

void AGGWorkshop::Press(const FVector2D& World)
{
	if (!bBuilding)
	{
		return;
	}
	bPressed = true;
	bStrokeMoved = false;
	PressWorld = DragWorld = World;
	bHover = true;
	HoverWorld = World;
	Held = EPick::None;
	HeldIndex = INDEX_NONE;
	StrokeBefore = Current.ToJson();

	int32 Index = INDEX_NONE;
	const EPick Pick = PickAt(World, Index);
	if (CurrentTool == EGGTool::Move)
	{
		if (Pick == EPick::None)
		{
			Say(TEXT("Nothing there to move. Ground and walls are repainted, not moved."));
			bPressed = false;
			return;
		}
		Held = Pick;
		HeldIndex = Index;
		HeldFrom = Pick == EPick::Gate ? Current.Gates[Index].Rect : FIntRect();
	}
	else if (CurrentTool == EGGTool::Erase && Pick != EPick::None)
	{
		bPressed = false;
		switch (Pick)
		{
		case EPick::Tee:
		case EPick::Cup:
			Say(TEXT("Every hole keeps its tee and its cup. Move them instead."));
			return;
		case EPick::Machine:
			Current.Machines.RemoveAt(Index);
			break;
		case EPick::Plate:
			RemovePlate(Index);
			break;
		case EPick::Pad:
			Current.Pads.RemoveAt(Index);
			break;
		case EPick::Gate:
			Current.Gates.RemoveAt(Index);
			break;
		default:
			break;
		}
		Changed(StrokeBefore);
	}
	else if (CurrentTool == EGGTool::Mirror && Pick == EPick::Machine && Current.Machines[Index].bMirror)
	{
		// Clicking a mirror turns it to its other diagonal.
		bPressed = false;
		Current.Machines[Index].Turn = 1 - Current.Machines[Index].Turn;
		Changed(StrokeBefore);
	}
	else if (CurrentTool == EGGTool::Gate && Pick == EPick::Gate)
	{
		// Clicking a gate hands it to the next plate.
		bPressed = false;
		FGGDesign::FGate& Gate = Current.Gates[Index];
		Gate.Plate = (Gate.Plate + 1) % FMath::Max(1, Current.Plates.Num());
		Say(FString::Printf(TEXT("That gate now opens with plate %d."), Gate.Plate + 1));
		Changed(StrokeBefore);
	}
}

void AGGWorkshop::Drag(const FVector2D& World)
{
	if (!bPressed || !bBuilding)
	{
		return;
	}
	DragWorld = World;
	bHover = true;
	HoverWorld = World;
	if (CurrentTool != EGGTool::Move || Held == EPick::None)
	{
		return;
	}
	// A held thing follows the pointer from square to square.
	const FIntPoint To = Spot(World);
	bool bMoved = false;
	auto MoveTo = [&](FIntPoint& At) {
		bMoved = At != To;
		At = To;
	};
	switch (Held)
	{
	case EPick::Tee:
		MoveTo(Current.Tee);
		break;
	case EPick::Cup:
		MoveTo(Current.Cup);
		break;
	case EPick::Machine:
		MoveTo(Current.Machines[HeldIndex].At);
		break;
	case EPick::Plate:
		MoveTo(Current.Plates[HeldIndex].At);
		break;
	case EPick::Pad:
		MoveTo(Current.Pads[HeldIndex]);
		break;
	case EPick::Gate:
	{
		const FIntPoint Shift = (Square(World) - Square(PressWorld)) * FGGDesign::Cell;
		const FIntRect Moved(HeldFrom.Min + Shift, HeldFrom.Max + Shift);
		bMoved = Moved != Current.Gates[HeldIndex].Rect;
		Current.Gates[HeldIndex].Rect = Moved;
		break;
	}
	default:
		break;
	}
	if (bMoved)
	{
		bStrokeMoved = true;
		Rebuild();
	}
}

void AGGWorkshop::Cancel()
{
	if (bPressed && bStrokeMoved)
	{
		// Put back what a half-finished move had shifted.
		Current.FromJson(StrokeBefore);
		Rebuild();
	}
	bPressed = false;
	bStrokeMoved = false;
	Held = EPick::None;
}

bool AGGWorkshop::Dragging(FIntRect& OutRect) const
{
	const bool bRectTool = CurrentTool == EGGTool::Floor || CurrentTool == EGGTool::Sand || CurrentTool == EGGTool::Wall
		|| CurrentTool == EGGTool::Soft || CurrentTool == EGGTool::Gate || CurrentTool == EGGTool::Erase;
	if (!bPressed || !bRectTool)
	{
		return false;
	}
	const FIntPoint A = Square(PressWorld);
	const FIntPoint B = Square(DragWorld);
	OutRect = FIntRect(FMath::Min(A.X, B.X) * FGGDesign::Cell, FMath::Min(A.Y, B.Y) * FGGDesign::Cell,
		(FMath::Max(A.X, B.X) + 1) * FGGDesign::Cell, (FMath::Max(A.Y, B.Y) + 1) * FGGDesign::Cell);
	return true;
}

bool AGGWorkshop::Hover(FVector2D& OutWorld) const
{
	OutWorld = HoverWorld;
	return bHover && bBuilding;
}

void AGGWorkshop::Release()
{
	if (!bPressed || !bBuilding)
	{
		bPressed = false;
		return;
	}
	FIntRect Rect;
	const bool bRect = Dragging(Rect);
	bPressed = false;
	const FIntRect Squares(Rect.Min / FGGDesign::Cell, Rect.Max / FGGDesign::Cell);
	const FIntPoint At = Spot(DragWorld);
	const EGGCell Under = Current.AtPoint(At);
	const bool bOpenGround = Under == EGGCell::Floor || Under == EGGCell::Sand;
	auto NeedsGround = [&]() {
		if (!bOpenGround)
		{
			Say(TEXT("That goes on open ground. Lay some ground there first."));
		}
		return bOpenGround;
	};
	switch (CurrentTool)
	{
	case EGGTool::Move:
		if (bStrokeMoved)
		{
			bStrokeMoved = false;
			Changed(StrokeBefore);
		}
		Held = EPick::None;
		return;
	case EGGTool::Floor:
		Current.Paint(Squares, EGGCell::Floor);
		break;
	case EGGTool::Sand:
		Current.Paint(Squares, EGGCell::Sand);
		break;
	case EGGTool::Wall:
		Current.Paint(Squares, EGGCell::Wall);
		break;
	case EGGTool::Soft:
		Current.Paint(Squares, EGGCell::Soft);
		break;
	case EGGTool::Erase:
		Current.Paint(Squares, EGGCell::None);
		break;
	case EGGTool::Gate:
		if (Current.Plates.Num() == 0)
		{
			Say(TEXT("A gate needs a plate to open it. Place a light plate or a weight plate first."));
			return;
		}
		if (Current.Gates.Num() >= 8)
		{
			Say(TEXT("Eight gates is the most a hole can have."));
			return;
		}
		Current.Gates.Add({Rect, Current.Plates.Num() - 1});
		Say(FString::Printf(TEXT("This gate opens with plate %d. Click it with the gate tool to give it another."), Current.Plates.Num()));
		break;
	case EGGTool::Pad:
		if (!NeedsGround())
		{
			return;
		}
		if (Current.Pads.Num() >= 3)
		{
			Say(TEXT("Three safe pads is the most a hole can have."));
			return;
		}
		Current.Pads.Add(At);
		break;
	case EGGTool::Plate:
	case EGGTool::Weight:
		if (!NeedsGround())
		{
			return;
		}
		if (Current.Plates.Num() >= 6)
		{
			Say(TEXT("Six plates is the most a hole can have."));
			return;
		}
		Current.Plates.Add({At, CurrentTool == EGGTool::Weight});
		break;
	case EGGTool::Pull:
	case EGGTool::Push:
		if (!NeedsGround())
		{
			return;
		}
		if (Current.Machines.Num() >= 4)
		{
			Say(TEXT("Four machines is the most a hole can have."));
			return;
		}
		Current.Machines.Add({At, CurrentTool == EGGTool::Push});
		break;
	case EGGTool::Mirror:
		if (!NeedsGround())
		{
			return;
		}
		if (Current.Machines.Num() >= 4)
		{
			Say(TEXT("Four machines is the most a hole can have."));
			return;
		}
		Current.Machines.Add({At, false, true, 0});
		break;
	default:
		return;
	}
	if (bRect || CurrentTool >= EGGTool::Pad)
	{
		if (Current.ToJson() != StrokeBefore)
		{
			Changed(StrokeBefore);
		}
	}
}

void AGGWorkshop::HandleTouch(int32 Finger, int32 Phase, const FVector2D& At)
{
	APlayerController* PC = Player();
	AGGHUD* Hud = PC ? Cast<AGGHUD>(PC->GetHUD()) : nullptr;
	if (!bBuilding || !Hud)
	{
		return;
	}
	FVector2D World;
	if (Phase == 0)
	{
		Fingers.Add(Finger, At);
		if (Fingers.Num() == 1)
		{
			bTwoFingers = false;
			if (!Hud->WorkshopPress(At) && WorldAt(At, World))
			{
				Press(World);
			}
		}
		else
		{
			// A second finger: this is a move of the view, not a stroke of the tool.
			Cancel();
			bTwoFingers = true;
		}
	}
	else if (Phase == 1)
	{
		if (bTwoFingers && Fingers.Num() >= 2 && Fingers.Contains(Finger))
		{
			auto Middle = [&](FVector2D& OutMiddle, double& OutSpread) {
				TArray<FVector2D> Points;
				Fingers.GenerateValueArray(Points);
				OutMiddle = (Points[0] + Points[1]) * 0.5;
				OutSpread = FMath::Max(FVector2D::Distance(Points[0], Points[1]), 1.0);
			};
			FVector2D Before;
			FVector2D After;
			double SpreadBefore = 1.0;
			double SpreadAfter = 1.0;
			Middle(Before, SpreadBefore);
			Fingers.Add(Finger, At);
			Middle(After, SpreadAfter);
			FVector2D WorldBefore;
			FVector2D WorldAfter;
			if (WorldAt(Before, WorldBefore) && WorldAt(After, WorldAfter))
			{
				Focus += WorldBefore - WorldAfter;
			}
			Distance *= SpreadBefore / SpreadAfter;
			PlaceCamera();
		}
		else
		{
			Fingers.Add(Finger, At);
			if (bPressed && WorldAt(At, World))
			{
				Drag(World);
			}
		}
	}
	else
	{
		Fingers.Remove(Finger);
		if (bPressed && !bTwoFingers)
		{
			Release();
		}
		if (Fingers.Num() == 0)
		{
			bTwoFingers = false;
			bHover = false;
		}
	}
}

void AGGWorkshop::PollInput(float DeltaSeconds)
{
	APlayerController* PC = Player();
	AGGHUD* Hud = PC ? Cast<AGGHUD>(PC->GetHUD()) : nullptr;
	const AGGGolfer* Pawn = Golfer();
	if (!PC || !Hud || Hud->MenuOpen())
	{
		Cancel();
		bPanning = false;
		return;
	}

	// Keys and the gamepad move the view: W A S D or the left stick slide it, the wheel or the triggers zoom.
	FVector2D Slide(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX));
	Slide.X += (PC->IsInputKeyDown(EKeys::W) ? 1.0 : 0.0) - (PC->IsInputKeyDown(EKeys::S) ? 1.0 : 0.0);
	Slide.Y += (PC->IsInputKeyDown(EKeys::D) ? 1.0 : 0.0) - (PC->IsInputKeyDown(EKeys::A) ? 1.0 : 0.0);
	const bool bPad = FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX)) + FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY)) > 0.2
		|| PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
	if (Slide.SizeSquared() > 0.04)
	{
		// Up the screen is -Y, right is +X.
		Focus += FVector2D(Slide.Y, -Slide.X) * (Distance * 0.75 * DeltaSeconds);
	}
	double Zoom = PC->GetInputAnalogKeyState(EKeys::MouseWheelAxis) * 3.0
		+ (PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) - PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis)) * 40.0 * DeltaSeconds;
	Zoom += (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) ? 3.0 : 0.0) - (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) ? 3.0 : 0.0);
	if (Zoom != 0.0)
	{
		Distance *= FMath::Pow(0.96, FMath::Clamp(Zoom, -12.0, 12.0));
	}
	if (PC->WasInputKeyJustPressed(EKeys::Z) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left))
	{
		Undo();
	}
	const int32 Step = (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder) ? 1 : 0)
		- (PC->WasInputKeyJustPressed(EKeys::Q) || PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftShoulder) ? 1 : 0);
	if (Step != 0)
	{
		const int32 Count = static_cast<int32>(EGGTool::Count);
		SetTool(static_cast<EGGTool>((static_cast<int32>(CurrentTool) + Step + Count) % Count));
	}
	if (PC->WasInputKeyJustPressed(EKeys::F))
	{
		FrameDesign();
	}
	PlaceCamera();
	if (Pawn && Pawn->UsingTouch())
	{
		// Fingers come through HandleTouch.
		return;
	}

	int32 SizeX = 0;
	int32 SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	float MouseX = 0.f;
	float MouseY = 0.f;
	bPadMode = bPad || (bPadMode && !(PC->GetMousePosition(MouseX, MouseY) && !FVector2D(MouseX, MouseY).Equals(LastMouse, 2.0)));
	// With a gamepad the pointer is the middle of the screen, and the view slides under it.
	const bool bMouse = bPadMode ? SizeX > 0 : PC->GetMousePosition(MouseX, MouseY);
	if (!bPadMode && bMouse)
	{
		LastMouse = FVector2D(MouseX, MouseY);
	}
	const FVector2D Screen = bPadMode ? FVector2D(SizeX * 0.5, SizeY * 0.5) : FVector2D(MouseX, MouseY);
	FVector2D World;
	const bool bOnGround = bMouse && WorldAt(Screen, World);
	bHover = bOnGround && (bPadMode || !Hud->WorkshopOver(Screen));
	HoverWorld = World;

	const FKey Button = bPadMode ? EKeys::Gamepad_FaceButton_Bottom : EKeys::LeftMouseButton;
	if (PC->WasInputKeyJustPressed(Button) && bMouse)
	{
		if ((bPadMode || !Hud->WorkshopPress(Screen)) && bOnGround)
		{
			Press(World);
		}
	}
	else if (bPressed && bOnGround)
	{
		Drag(World);
	}
	if (bPressed && !PC->IsInputKeyDown(Button))
	{
		Release();
	}

	// The right or middle button drags the view: the ground under the pointer stays under it.
	if (!bPadMode && bMouse && (PC->IsInputKeyDown(EKeys::RightMouseButton) || PC->IsInputKeyDown(EKeys::MiddleMouseButton)))
	{
		FVector2D From;
		if (bPanning && WorldAt(PanFrom, From) && bOnGround)
		{
			Focus += From - World;
			PlaceCamera();
		}
		bPanning = true;
		PanFrom = Screen;
	}
	else
	{
		bPanning = false;
	}
}

// ---- keeping, sharing

void AGGWorkshop::NewHole()
{
	const FString Before = Current.ToJson();
	const UGGSave& Save = UGGSave::Get();
	Current = FGGDesign::Starter();
	for (int32 Number = 1; Number < 100; ++Number)
	{
		Current.Name = Number == 1 ? FString(TEXT("My hole")) : FString::Printf(TEXT("My hole %d"), Number);
		if (!Save.Designs.Contains(Current.Name))
		{
			break;
		}
	}
	Changed(Before);
	FrameDesign();
	Say(TEXT("A new hole. It starts as a small puzzle: change anything."));
}

void AGGWorkshop::NameHole(const FString& Name)
{
	Current.Name = Name.Left(28);
	UGGSave::Get().Draft = Current.ToJson();
	bDraftDirty = true;
}

void AGGWorkshop::SaveHole()
{
	UGGSave& Save = UGGSave::Get();
	if (Current.Name.TrimStartAndEnd().IsEmpty())
	{
		Current.Name = TEXT("My hole");
	}
	if (!Save.Designs.Contains(Current.Name) && Save.Designs.Num() >= 40)
	{
		Say(TEXT("Forty holes are kept at most. Delete one first."));
		return;
	}
	Save.Designs.Add(Current.Name, Current.ToJson());
	Save.Draft = Current.ToJson();
	UGGSave::Store();
	bDraftDirty = false;
	Say(FString::Printf(TEXT("Kept as \"%s\"."), *Current.Name));
}

bool AGGWorkshop::LoadJson(const FString& Json)
{
	const FString Before = Current.ToJson();
	FGGDesign Loaded;
	if (!Loaded.FromJson(Json))
	{
		return false;
	}
	const TArray<FGGSolution> Answers = Loaded.Solutions;
	const int32 Necessary = Loaded.Necessary;
	const int32 Luck = Loaded.Luck;
	Current = MoveTemp(Loaded);
	Changed(Before);
	// What was proven about it when it was kept still holds, if its answers still play out.
	Current.Necessary = Necessary;
	Current.Luck = Luck;
	Current.Solutions.Reset();
	if (Hole)
	{
		const GG::FSetup Setup = Hole->MakeSetup();
		for (const FGGSolution& Answer : Answers)
		{
			if (Verify(Setup, Answer))
			{
				Current.Solutions.Add(Answer);
			}
		}
		Rebuild();
	}
	UGGSave::Get().Draft = Current.ToJson();
	if (!bBuilding)
	{
		StartBuilding();
	}
	FrameDesign();
	return true;
}

bool AGGWorkshop::LoadHole(const FString& Name)
{
	const FString* Json = UGGSave::Get().Designs.Find(Name);
	if (Json && LoadJson(*Json))
	{
		Say(FString::Printf(TEXT("Opened \"%s\"."), *Current.Name));
		return true;
	}
	Say(TEXT("That hole couldn't be opened."));
	return false;
}

void AGGWorkshop::DeleteHole(const FString& Name)
{
	UGGSave& Save = UGGSave::Get();
	if (Save.Designs.Remove(Name) > 0)
	{
		UGGSave::Store();
		Say(FString::Printf(TEXT("Deleted \"%s\". What is on the table stays until you change it."), *Name));
	}
}

FString AGGWorkshop::ShareCode() const
{
	// A short line of text: the design's squares as rectangles, then everything that stands on them. Small enough
	// to paste into a chat. (The proof is not part of it: whoever opens the code checks the hole for themselves.)
	FString Code = TEXT("GG1");
	auto AddRects = [&](TCHAR Tag, EGGCell Kind) {
		for (const FIntRect& R : Current.Rects(Kind))
		{
			Code += FString::Printf(TEXT(";%c%d,%d,%d,%d"), Tag, R.Min.X / FGGDesign::Cell, R.Min.Y / FGGDesign::Cell, R.Width() / FGGDesign::Cell,
				R.Height() / FGGDesign::Cell);
		}
	};
	auto SquareOf = [](const FIntPoint& At) { return FIntPoint(FMath::FloorToInt32(double(At.X) / FGGDesign::Cell), FMath::FloorToInt32(double(At.Y) / FGGDesign::Cell)); };
	Code += TEXT(";n") + Current.Name.Replace(TEXT(" "), TEXT("_")).Replace(TEXT(";"), TEXT(""));
	AddRects(TEXT('f'), EGGCell::Floor);
	AddRects(TEXT('s'), EGGCell::Sand);
	AddRects(TEXT('w'), EGGCell::Wall);
	AddRects(TEXT('h'), EGGCell::Soft);
	Code += FString::Printf(TEXT(";t%d,%d;c%d,%d"), SquareOf(Current.Tee).X, SquareOf(Current.Tee).Y, SquareOf(Current.Cup).X, SquareOf(Current.Cup).Y);
	for (const FIntPoint& Pad : Current.Pads)
	{
		Code += FString::Printf(TEXT(";p%d,%d"), SquareOf(Pad).X, SquareOf(Pad).Y);
	}
	for (const FGGDesign::FPlate& Plate : Current.Plates)
	{
		Code += FString::Printf(TEXT(";%c%d,%d"), Plate.bWeight ? TEXT('k') : TEXT('l'), SquareOf(Plate.At).X, SquareOf(Plate.At).Y);
	}
	for (const FGGDesign::FGate& Gate : Current.Gates)
	{
		Code += FString::Printf(TEXT(";g%d,%d,%d,%d,%d"), Gate.Rect.Min.X / FGGDesign::Cell, Gate.Rect.Min.Y / FGGDesign::Cell,
			Gate.Rect.Width() / FGGDesign::Cell, Gate.Rect.Height() / FGGDesign::Cell, Gate.Plate);
	}
	for (const FGGDesign::FMachine& Machine : Current.Machines)
	{
		if (Machine.bMirror)
		{
			Code += FString::Printf(TEXT(";m%d,%d,%d"), SquareOf(Machine.At).X, SquareOf(Machine.At).Y, Machine.Turn);
			continue;
		}
		Code += FString::Printf(TEXT(";%c%d,%d"), Machine.bPush ? TEXT('r') : TEXT('a'), SquareOf(Machine.At).X, SquareOf(Machine.At).Y);
	}
	return Code;
}

bool AGGWorkshop::ImportCode(const FString& Code)
{
	TArray<FString> Parts;
	Code.TrimStartAndEnd().ParseIntoArray(Parts, TEXT(";"));
	if (Parts.Num() < 3 || Parts[0] != TEXT("GG1"))
	{
		Say(TEXT("That isn't a Gravity Golf hole code."));
		return false;
	}
	FGGDesign Design;
	bool bTee = false;
	bool bCup = false;
	for (int32 I = 1; I < Parts.Num() && I < 4000; ++I)
	{
		const FString& Part = Parts[I];
		const TCHAR Tag = Part[0];
		if (Tag == TEXT('n'))
		{
			Design.Name = Part.Mid(1).Replace(TEXT("_"), TEXT(" ")).Left(28);
			continue;
		}
		TArray<FString> Fields;
		Part.Mid(1).ParseIntoArray(Fields, TEXT(","));
		TArray<int32> N;
		for (const FString& Field : Fields)
		{
			N.Add(FMath::Clamp(FCString::Atoi(*Field), -MaxExtent, MaxExtent));
		}
		const auto Middle = [&]() { return FIntPoint(N[0] * FGGDesign::Cell + FGGDesign::Cell / 2, N[1] * FGGDesign::Cell + FGGDesign::Cell / 2); };
		const bool bRect = N.Num() >= 4 && N[2] > 0 && N[3] > 0;
		const bool bPoint = N.Num() >= 2;
		if (bRect && (Tag == TEXT('f') || Tag == TEXT('s') || Tag == TEXT('w') || Tag == TEXT('h')))
		{
			Design.Paint(FIntRect(N[0], N[1], N[0] + N[2], N[1] + N[3]),
				Tag == TEXT('f') ? EGGCell::Floor : Tag == TEXT('s') ? EGGCell::Sand : Tag == TEXT('w') ? EGGCell::Wall : EGGCell::Soft);
		}
		else if (bPoint && Tag == TEXT('t'))
		{
			Design.Tee = Middle();
			bTee = true;
		}
		else if (bPoint && Tag == TEXT('c'))
		{
			Design.Cup = Middle();
			bCup = true;
		}
		else if (bPoint && Tag == TEXT('p') && Design.Pads.Num() < 3)
		{
			Design.Pads.Add(Middle());
		}
		else if (bPoint && (Tag == TEXT('l') || Tag == TEXT('k')) && Design.Plates.Num() < 6)
		{
			Design.Plates.Add({Middle(), Tag == TEXT('k')});
		}
		else if (bRect && N.Num() >= 5 && Tag == TEXT('g') && Design.Plates.IsValidIndex(N[4]) && Design.Gates.Num() < 8)
		{
			Design.Gates.Add({FIntRect(N[0] * FGGDesign::Cell, N[1] * FGGDesign::Cell, (N[0] + N[2]) * FGGDesign::Cell, (N[1] + N[3]) * FGGDesign::Cell), N[4]});
		}
		else if (bPoint && (Tag == TEXT('a') || Tag == TEXT('r')) && Design.Machines.Num() < 4)
		{
			Design.Machines.Add({Middle(), Tag == TEXT('r')});
		}
		else if (bPoint && Tag == TEXT('m') && Design.Machines.Num() < 4)
		{
			Design.Machines.Add({Middle(), false, true, N.Num() >= 3 && N[2] != 0 ? 1 : 0});
		}
	}
	if (!bTee || !bCup || Design.Cells.Num() == 0)
	{
		Say(TEXT("That hole code is damaged."));
		return false;
	}
	if (!LoadJson(Design.ToJson()))
	{
		return false;
	}
	Say(FString::Printf(TEXT("Opened \"%s\" from its code. Test it, and check it."), *Current.Name));
	return true;
}

// ---- the proof

bool AGGWorkshop::Verify(const GG::FSetup& Setup, const FGGSolution& Answer)
{
	if (Answer.Machines.Num() != static_cast<int32>(Setup.Machines.size()) || Answer.Leg < 0 || Answer.Leg > static_cast<int32>(Setup.Course.SafeZones.size()))
	{
		return false;
	}
	GG::FLeg Leg = GG::MakeLeg(Setup, Answer.Leg);
	for (int32 I = 0; I < Answer.Held.Num() && I < static_cast<int32>(Leg.Held.size()); ++I)
	{
		Leg.Held[I] = Answer.Held[I];
	}
	std::vector<GG::FAttractor> Machines;
	for (size_t I = 0; I < Setup.Machines.size(); ++I)
	{
		Machines.push_back(GG::Stand(Setup.Machines[I], Answer.Machines[I].X, Answer.Machines[I].Y, Answer.Turns.IsValidIndex(I) ? Answer.Turns[I] : 0));
	}
	return GG::LegReached(Setup, Leg, Machines, GG::LaunchVelocity(Answer.Yaw, Answer.Power));
}

void AGGWorkshop::CollectAnswers()
{
	if (!Hole || Hole->PlayedSerial() == SeenPlayed)
	{
		return;
	}
	SeenPlayed = Hole->PlayedSerial();
	// A leg the designer has just played out is an answer to it, if it plays out the same when replayed from its
	// numbers (a swing that only just works may not).
	const GG::FSetup Setup = Hole->MakeSetup();
	for (const FGGSolution& Answer : Hole->PlayedLegs())
	{
		const bool bKnown = Current.Solutions.ContainsByPredicate([&](const FGGSolution& Old) {
			return Old.Leg == Answer.Leg && Old.Yaw == Answer.Yaw && Old.Power == Answer.Power;
		});
		if (bKnown)
		{
			continue;
		}
		if (Verify(Setup, Answer))
		{
			Current.Solutions.RemoveAll([&](const FGGSolution& Old) { return Old.Leg == Answer.Leg; });
			Current.Solutions.Add(Answer);
			UE_LOG(LogGGWorkshop, Display, TEXT("Workshop: leg %d answered (yaw %.2f, power %.3f)"), Answer.Leg + 1, Answer.Yaw, Answer.Power);
		}
		else
		{
			Hole->Notify(TEXT("That one was too fine to count as the answer. Leave more room."));
		}
	}
	UGGSave::Get().Draft = Current.ToJson();
	bDraftDirty = true;
}

void AGGWorkshop::StartProof()
{
	if (!Hole || bProving || bRating)
	{
		return;
	}
	if (Current.Machines.Num() == 0)
	{
		Current.Necessary = -1;
		Say(TEXT("There is no machine in this hole, so there is nothing to work out. Add an attractor or a repulsor."));
		return;
	}
	ProofSetup = Hole->MakeSetup();
	// Every leg, from every state its plates could be in when the ball gets there: none pressed yet, all of the
	// light plates pressed, and the state the designer's own answer had.
	ProofCases.Reset();
	const int32 Legs = Current.Legs();
	const int32 Switches = static_cast<int32>(ProofSetup.Course.Switches.size());
	for (int32 Leg = 0; Leg < Legs; ++Leg)
	{
		TArray<TArray<bool>> States;
		TArray<bool> NoneHeld;
		NoneHeld.Init(false, Switches);
		States.Add(NoneHeld);
		if (Leg > 0)
		{
			TArray<bool> All;
			for (int32 I = 0; I < Switches; ++I)
			{
				All.Add(!ProofSetup.Course.Switches[I].bWeight);
			}
			States.AddUnique(All);
			for (const FGGSolution& Answer : Current.Solutions)
			{
				if (Answer.Leg == Leg && Answer.Held.Num() == Switches)
				{
					States.AddUnique(Answer.Held);
				}
			}
		}
		for (const TArray<bool>& State : States)
		{
			ProofCases.Add({Leg, State});
		}
	}
	ProofCase = 0;
	ProofRow = 0;
	bProving = true;
	Say(TEXT("Checking every swing without the machines..."));
}

float AGGWorkshop::ProofProgress() const
{
	// The check is the first half of the bar, the rating the second.
	if (bRating)
	{
		return 0.5f + 0.5f * static_cast<float>(RateLeg * RatePlacements + RatePlacement) / static_cast<float>(Current.Legs() * RatePlacements);
	}
	const float Checked = ProofCases.Num() > 0 ? static_cast<float>(ProofCase * ProofRows + ProofRow) / static_cast<float>(ProofCases.Num() * ProofRows) : 0.f;
	return bProving ? Checked * 0.5f : Checked;
}

void AGGWorkshop::StepProof()
{
	// A few thousand swings a frame, every one with the real physics and no machine. One that completes its leg
	// means the hole can be done by aim alone, which the puzzle law forbids.
	const double Started = FPlatformTime::Seconds();
	const FProofCase& Case = ProofCases[ProofCase];
	GG::FLeg Leg = GG::MakeLeg(ProofSetup, Case.Leg);
	for (int32 I = 0; I < Case.Held.Num(); ++I)
	{
		Leg.Held[I] = Case.Held[I];
	}
	const int32 Rows = FMath::Min(ProofBatch, ProofRows - ProofRow);
	std::atomic<int32> Hit{-1};
	const GG::FSetup& Setup = ProofSetup;
	const int32 First = ProofRow;
	ParallelFor(Rows, [&](int32 Row) {
		const double Yaw = -180.0 + 360.0 * (First + Row) / ProofRows;
		for (int32 P = 0; P < ProofPowers; ++P)
		{
			if (GG::LegReached(Setup, Leg, {}, GG::LaunchVelocity(Yaw, 0.05 + 0.0125 * P)))
			{
				Hit = (First + Row) * 1000 + P;
				return;
			}
		}
	});
	ProofRow += Rows;
	const double Took = FPlatformTime::Seconds() - Started;
	ProofBatch = Took < 0.004 ? FMath::Min(ProofBatch * 2, ProofRows) : Took > 0.012 ? FMath::Max(ProofBatch / 2, 1) : ProofBatch;

	if (Hit >= 0)
	{
		const double Yaw = -180.0 + 360.0 * (Hit / 1000) / ProofRows;
		const double Power = 0.05 + 0.0125 * (Hit % 1000);
		bProving = false;
		Current.Necessary = -1;
		ProofFlawFrom = FVector2D(Leg.Start.X, Leg.Start.Y);
		ProofFlawYaw = Yaw;
		bProofFlaw = true;
		Say(FString::Printf(TEXT("Not a puzzle yet: on swing %d a plain shot does it, with no machine (the line shown, at %d%% power). Block it."),
			Case.Leg + 1, FMath::RoundToInt32(Power * 100.0)));
	}
	else if (ProofRow >= ProofRows)
	{
		ProofRow = 0;
		if (++ProofCase >= ProofCases.Num())
		{
			bProving = false;
			Current.Necessary = 1;
			bProofFlaw = false;
			StartRating();
		}
	}
	if (!bProving)
	{
		UGGSave::Get().Draft = Current.ToJson();
		bDraftDirty = true;
	}
}

void AGGWorkshop::StartRating()
{
	// The second question the puzzle law asks: does it matter where the machines go? Stand them at random, many
	// times over, and see how often some swing still works. The hole is as hard as its hardest swing.
	RateGround.Reset();
	for (const TPair<FIntPoint, EGGCell>& Pair : Current.Cells)
	{
		const FIntPoint At(Pair.Key.X * FGGDesign::Cell + FGGDesign::Cell / 2, Pair.Key.Y * FGGDesign::Cell + FGGDesign::Cell / 2);
		if ((Pair.Value == EGGCell::Floor || Pair.Value == EGGCell::Sand) && GG::MachineAllowed(ProofSetup, At.X, At.Y))
		{
			RateGround.Add(At);
		}
	}
	// The same hole is always rated from the same placements.
	RateGround.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y != B.Y ? A.Y < B.Y : A.X < B.X; });
	// (Seeded from the hole's code without its name, so renaming a hole does not change its rating.)
	FString Shape = ShareCode();
	int32 NameAt = INDEX_NONE;
	int32 NameEnd = INDEX_NONE;
	if (Shape.FindChar(TEXT(';'), NameAt) && Shape.Mid(NameAt + 1).FindChar(TEXT(';'), NameEnd))
	{
		Shape.RemoveAt(NameAt, NameEnd + 1);
	}
	RateRandom.Initialize(static_cast<int32>(FCrc::StrCrc32(*Shape)));
	if (RateGround.Num() == 0)
	{
		Say(TEXT("No swing does it without the machines."));
		return;
	}
	RateLeg = 0;
	RatePlacement = 0;
	RateHits = 0;
	RateWorst = 101;
	bRating = true;
	NextPlacement();
	Say(TEXT("No swing does it without the machines. Now trying the machines in random places..."));
}

void AGGWorkshop::NextPlacement()
{
	RateMachines.clear();
	for (const GG::FMachineDef& Machine : ProofSetup.Machines)
	{
		const FIntPoint At = RateGround[RateRandom.RandRange(0, RateGround.Num() - 1)];
		// (A mirror is turned one way or the other at random.)
		RateMachines.push_back(GG::Stand(Machine, At.X, At.Y, RateRandom.RandRange(0, 1)));
	}
	RateRow = 0;
}

void AGGWorkshop::StepRating()
{
	const double Started = FPlatformTime::Seconds();
	while (bRating && FPlatformTime::Seconds() - Started < 0.006)
	{
		GG::FLeg Leg = GG::MakeLeg(ProofSetup, RateLeg);
		for (const FGGSolution& Answer : Current.Solutions)
		{
			// The plates as they stood when the designer played this swing.
			if (Answer.Leg == RateLeg && Answer.Held.Num() == static_cast<int32>(Leg.Held.size()))
			{
				for (int32 I = 0; I < Answer.Held.Num(); ++I)
				{
					Leg.Held[I] = Answer.Held[I];
				}
			}
		}
		const int32 Rows = FMath::Min(RateBatch, RateRows - RateRow);
		std::atomic<bool> Hit{false};
		const GG::FSetup& Setup = ProofSetup;
		const std::vector<GG::FAttractor>& Machines = RateMachines;
		const int32 First = RateRow;
		const double SliceStarted = FPlatformTime::Seconds();
		ParallelFor(Rows, [&](int32 Row) {
			const double Yaw = -180.0 + 360.0 * (First + Row) / RateRows;
			for (int32 P = 0; P < RatePowers && !Hit; ++P)
			{
				if (GG::LegReached(Setup, Leg, Machines, GG::LaunchVelocity(Yaw, 0.05 + 0.05 * P)))
				{
					Hit = true;
				}
			}
		});
		const double Took = FPlatformTime::Seconds() - SliceStarted;
		RateBatch = Took < 0.002 ? FMath::Min(RateBatch * 2, RateRows) : Took > 0.006 ? FMath::Max(RateBatch / 2, 1) : RateBatch;
		RateRow += Rows;
		if (!Hit && RateRow < RateRows)
		{
			continue;
		}
		RateHits += Hit ? 1 : 0;
		if (++RatePlacement >= RatePlacements)
		{
			RateWorst = FMath::Min(RateWorst, FMath::RoundToInt32(100.0 * RateHits / RatePlacements));
			RatePlacement = 0;
			RateHits = 0;
			if (++RateLeg >= Current.Legs())
			{
				bRating = false;
				Current.Luck = RateWorst;
				if (Current.Luck == 0 && !Current.Solved())
				{
					Say(TEXT("No swing does it without the machines, and no random placement of them works either. It is very hard, or impossible: solve it to show it can be done."));
				}
				else
				{
					const TCHAR* Verdict = Current.Luck > 35 ? TEXT("Almost anywhere works: make where the machines go matter.")
						: Current.Solved() ? TEXT("And you have solved it. This is a puzzle.") : TEXT("Now test it and solve it yourself to finish the proof.");
					Say(FString::Printf(TEXT("No swing does it without the machines. Rated %s: %d in 100 random placements can be made to work. %s"),
						FGGDesign::RatingName(Current.Luck), Current.Luck, Verdict));
				}
				UGGSave::Get().Draft = Current.ToJson();
				bDraftDirty = true;
				return;
			}
		}
		NextPlacement();
	}
}

bool AGGWorkshop::ProofFlaw(FVector2D& OutFrom, double& OutYaw) const
{
	OutFrom = ProofFlawFrom;
	OutYaw = ProofFlawYaw;
	return bProofFlaw && Current.Necessary == -1;
}

void AGGWorkshop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (NoteTime > 0.0)
	{
		NoteTime -= DeltaSeconds;
		if (NoteTime <= 0.0 && !bProving)
		{
			Note.Reset();
		}
	}
	if (bBuilding)
	{
		// Hold the view from above (a level that has just opened may not have had a player yet).
		APlayerController* PC = Player();
		if (PC && PC->GetViewTarget() != this)
		{
			StartBuilding();
		}
		PollInput(DeltaSeconds);
		PlaceCamera();
		if (bProving)
		{
			StepProof();
		}
		else if (bRating)
		{
			StepRating();
		}
	}
	else
	{
		CollectAnswers();
	}
	if (bDraftDirty)
	{
		DraftAge += DeltaSeconds;
		if (DraftAge > 4.0)
		{
			// Keep the work on the table safe without writing to disk on every stroke.
			UGGSave::Store();
			bDraftDirty = false;
			DraftAge = 0.0;
		}
	}
}
