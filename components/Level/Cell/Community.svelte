<script lang="ts">
	import type CommunityLevelRecord from '$lib/level/community/record'
	import Base from './Base.svelte'

	export let level: CommunityLevelRecord
</script>

<Base href="/levels/community/{level.id}" stars={null}>
	{level.name}
</Base>
