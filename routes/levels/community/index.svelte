<script context="module" lang="ts">
	export const load: Load = ({ fetch }) => _load(fetch)
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import CommunityLevelsPage, {
		load as _load
	} from '../../../components/Level/Page/CommunityLevels.svelte'
</script>

<CommunityLevelsPage />
