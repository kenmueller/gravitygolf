#pragma once

#include "CoreMinimal.h"
#include "Game/GGCourse.h"
#include "GameFramework/Actor.h"

#include "GGWorkshop.generated.h"

class AGGGolfer;
class UCameraComponent;

/** What the pointer does while building. */
enum class EGGTool : uint8
{
	Move,
	Floor,
	Sand,
	Wall,
	Soft,
	Pad,
	Plate,
	Weight,
	Gate,
	Pull,
	Push,
	Erase,
	Count,
};

/** What stands on one 50 cm square of a design. Every kind but None has floor under it. */
enum class EGGCell : uint8
{
	None,
	Floor,
	Sand,
	Wall,
	Soft,
};

/**
 * A hole being designed. Ground and walls are painted on a grid of 50 cm squares; everything else stands on the
 * grid's corners. Positions are whole centimetres. It is saved as the same course JSON the game's own holes use.
 */
struct FGGDesign
{
	static constexpr int32 Cell = 50;

	struct FPlate
	{
		FIntPoint At = FIntPoint::ZeroValue;
		bool bWeight = false;
	};
	struct FGate
	{
		FIntRect Rect;
		int32 Plate = 0;
	};
	struct FMachine
	{
		FIntPoint At = FIntPoint::ZeroValue;
		bool bPush = false;
	};

	FString Name = TEXT("My hole");
	/** Keyed by square: (x, y) is the square from (x, y) * 50 cm to (x + 1, y + 1) * 50 cm. */
	TMap<FIntPoint, EGGCell> Cells;
	FIntPoint Tee = FIntPoint::ZeroValue;
	FIntPoint Cup = FIntPoint::ZeroValue;
	TArray<FIntPoint> Pads;
	TArray<FPlate> Plates;
	TArray<FGate> Gates;
	TArray<FMachine> Machines;
	/** Legs the designer has played out since the last change: the proof that the hole can be solved. */
	TArray<FGGSolution> Solutions;
	/** Has it been checked that no swing works without the machines? 0 not checked, 1 proven, -1 it isn't true. */
	int32 Necessary = 0;
	/** How much placement matters: of a hundred random ways to stand the machines, how many can be made to work, on
	 *  the hole's hardest swing. Lower is harder. -1 until it has been rated (the check does it). */
	int32 Luck = -1;
	/** The rating in a word: Too easy, Easy, Medium, Hard, Fiendish. Empty until rated. */
	static const TCHAR* RatingName(int32 Luck);

	/** A plain strip of ground with a tee and a cup, to start from. */
	static FGGDesign Starter();
	EGGCell At(const FIntPoint& Square) const { return Cells.FindRef(Square); }
	EGGCell AtPoint(const FIntPoint& Point) const;
	void Paint(const FIntRect& Squares, EGGCell Kind);
	/** Squares of one kind (for Floor: every square with ground that is not sand) merged into rectangles, in centimetres. */
	TArray<FIntRect> Rects(EGGCell Kind) const;
	bool Bounds(FIntRect& Out) const;
	int32 Legs() const { return Pads.Num() + 1; }
	bool Solved() const;

	FString ToJson() const;
	bool FromJson(const FString& Text);
};

/**
 * The Workshop: where players build holes of their own. It owns the design, edits it with the tools, shows it from
 * above while building, and hands it to the course to be played. It also holds a design to the puzzle law: a hole is
 * "proven" once its designer has solved it and the game has checked that no swing works without the machines.
 */
UCLASS()
class GRAVITYGOLF_API AGGWorkshop : public AActor
{
	GENERATED_BODY()

public:
	AGGWorkshop();

	static AGGWorkshop* Get(const UObject* WorldContext);
	void Begin(AGGCourse* InCourse);
	virtual void Tick(float DeltaSeconds) override;

	bool Building() const { return bBuilding; }
	void StartBuilding();
	/** Play the hole as it stands. False, with a message, if it can't be played yet. */
	bool StartTesting();
	void Toggle();

	EGGTool Tool() const { return CurrentTool; }
	void SetTool(EGGTool Tool);
	static const TCHAR* ToolName(EGGTool Tool);
	static const TCHAR* ToolHelp(EGGTool Tool);

	// The pointer: a mouse or one finger. Press, drag and release in world centimetres on the ground.
	void Press(const FVector2D& World);
	void Drag(const FVector2D& World);
	void Release();
	void Cancel();
	/** A touch while building, in viewport pixels. Phase 0 is down, 1 moved, 2 up. One finger works the tool; two
	 *  move and zoom the view. */
	void HandleTouch(int32 Finger, int32 Phase, const FVector2D& At);
	/** Where a viewport position falls on the ground. */
	bool WorldAt(const FVector2D& Screen, FVector2D& OutWorld) const;
	void LookAt(const FVector2D& World, double Distance);
	void FrameDesign();

	void Undo();
	bool CanUndo() const { return History.Num() > 0; }
	void NewHole();
	void NameHole(const FString& Name);
	/** Keep the design under its name in the player's saved holes. */
	void SaveHole();
	bool LoadHole(const FString& Name);
	void DeleteHole(const FString& Name);
	/** The design as a line of text a friend can paste in. */
	FString ShareCode() const;
	bool ImportCode(const FString& Code);
	bool LoadJson(const FString& Json);

	/** Check that no swing sinks the ball without the machines. Runs a little each frame. */
	void StartProof();
	bool Proving() const { return bProving || bRating; }
	/** The second half of the check: trying random placements to see how much placement matters. */
	bool Rating() const { return bRating; }
	float ProofProgress() const;
	/** When the check failed: where the plain shot that works starts, and its direction, to draw it. */
	bool ProofFlaw(FVector2D& OutFrom, double& OutYaw) const;

	const FGGDesign& Design() const { return Current; }
	const FString& Message() const { return Note; }
	void Say(const FString& Text);
	/** The square or corner the pointer is over, for the HUD. */
	bool Hover(FVector2D& OutWorld) const;
	/** The rectangle being dragged out, in centimetres. */
	bool Dragging(FIntRect& OutRect) const;
	AGGCourse* Course() const { return Hole; }

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

private:
	enum class EPick : uint8
	{
		None,
		Tee,
		Cup,
		Machine,
		Plate,
		Pad,
		Gate,
	};

	AGGGolfer* Golfer() const;
	APlayerController* Player() const;
	/** Give the design to the course, as it is right now. */
	void Rebuild();
	/** Something changed: remember the state before for Undo, forget the proofs, and rebuild. */
	void Changed(const FString& Before);
	EPick PickAt(const FVector2D& World, int32& OutIndex) const;
	void Erase(const FVector2D& World);
	void RemovePlate(int32 Index);
	void PollInput(float DeltaSeconds);
	void StepProof();
	void StartRating();
	void StepRating();
	void NextPlacement();
	void CollectAnswers();
	void PlaceCamera();
	static FIntPoint Square(const FVector2D& World);
	/** The middle of the square a point is in: where things stand. */
	static FIntPoint Spot(const FVector2D& World);
	/** Does a recorded answer still complete its leg when replayed from its numbers? */
	static bool Verify(const GG::FSetup& Setup, const FGGSolution& Answer);

	UPROPERTY()
	TObjectPtr<AGGCourse> Hole;

	FGGDesign Current;
	TArray<FString> History;
	/** Move harms nothing: a first stray click or tap (the one that wakes touch controls, say) paints no wall. */
	EGGTool CurrentTool = EGGTool::Move;
	bool bBuilding = false;
	FString Note;
	double NoteTime = 0.0;

	// The pointer's stroke.
	bool bPressed = false;
	FVector2D PressWorld = FVector2D::ZeroVector;
	FVector2D DragWorld = FVector2D::ZeroVector;
	EPick Held = EPick::None;
	int32 HeldIndex = INDEX_NONE;
	FString StrokeBefore;
	FIntRect HeldFrom;
	bool bStrokeMoved = false;
	bool bHover = false;
	FVector2D HoverWorld = FVector2D::ZeroVector;

	// The view from above.
	FVector2D Focus = FVector2D::ZeroVector;
	double Distance = 3600.0;
	bool bPanning = false;
	FVector2D PanFrom = FVector2D::ZeroVector;
	TMap<int32, FVector2D> Fingers;
	bool bTwoFingers = false;
	bool bPadMode = false;
	FVector2D LastMouse = FVector2D(-1.0, -1.0);
	bool bDraftDirty = false;
	double DraftAge = 0.0;

	// The proof under way.
	bool bProving = false;
	GG::FSetup ProofSetup;
	struct FProofCase
	{
		int32 Leg = 0;
		TArray<bool> Held;
	};
	TArray<FProofCase> ProofCases;
	int32 ProofCase = 0;
	int32 ProofRow = 0;
	int32 ProofBatch = 8;
	bool bProofFlaw = false;
	FVector2D ProofFlawFrom = FVector2D::ZeroVector;
	double ProofFlawYaw = 0.0;
	int32 SeenPlayed = 0;

	// The rating under way: random placements of every machine, each tried with every swing.
	bool bRating = false;
	int32 RateLeg = 0;
	int32 RatePlacement = 0;
	int32 RateRow = 0;
	int32 RateHits = 0;
	int32 RateWorst = 101;
	int32 RateBatch = 4;
	std::vector<GG::FAttractor> RateMachines;
	TArray<FIntPoint> RateGround;
	FRandomStream RateRandom;
};
