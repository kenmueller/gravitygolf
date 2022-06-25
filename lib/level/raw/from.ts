import type RawLevel from '.'
import type Level from '..'

const levelFromRaw = ({
	gravity,
	antigravity,
	ball,
	hole,
	stars,
	walls
}: RawLevel): Level => ({
	maxGravity: Array.isArray(gravity) ? gravity[0] : gravity ?? 0,
	maxAntigravity: Array.isArray(antigravity)
		? antigravity[0]
		: antigravity ?? 0,
	defaultGravity: Array.isArray(gravity)
		? gravity[1].map(([x, y]) => ({ x, y }))
		: [],
	defaultAntigravity: Array.isArray(antigravity)
		? antigravity[1].map(([x, y]) => ({ x, y }))
		: [],
	ball: { x: ball[0], y: ball[1], radius: ball[2] },
	hole: { x: hole[0], y: hole[1], radius: hole[2] },
	stars: stars?.map(([x, y, radius]) => ({ x, y, radius })) ?? [],
	walls: walls?.map(([x, y, width, height]) => ({ x, y, width, height })) ?? []
})

export default levelFromRaw
