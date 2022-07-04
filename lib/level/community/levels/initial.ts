import { writable } from 'svelte/store'

import type CommunityLevelRecord from '../record'

const initialCommunityLevels = writable<CommunityLevelRecord[]>([])

export default initialCommunityLevels
