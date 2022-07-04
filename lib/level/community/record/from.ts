import type CommunityLevelRecord from '.'

const communityLevelFromRecord = (record: Record<string, unknown>) =>
	({
		id: record.objectID,
		name: record.name,
		gravity: record.gravity,
		antigravity: record.antigravity,
		stars: record.stars,
		attempts: record.attempts,
		completions: record.completions
	} as CommunityLevelRecord)

export default communityLevelFromRecord
