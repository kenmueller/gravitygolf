import type CommunityLevelRecord from '.'

const communityLevelFromRecord = (record: Record<string, unknown>) =>
	({
		id: record.objectID,
		name: record.name,
		gravity: record.gravity,
		antigravity: record.antigravity,
		stars: record.stars,
		attempts: record.attempts,
		wins: record.wins
	} as CommunityLevelRecord)

export default communityLevelFromRecord
