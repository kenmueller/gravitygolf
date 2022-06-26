<script context="module" lang="ts">
	export const load: Load = ({ params }) => {
		const id = Number.parseInt(params.id)

		if (Number.isNaN(id)) return { status: 301, redirect: '/levels' }

		if (id - 1 < 0) return { status: 301, redirect: '/levels' }
		if (id - 1 >= levels.length) return { status: 307, redirect: '/levels' }

		return {
			props: { id }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import { goto } from '$app/navigation'

	import levels from '$lib/level/levels'
	import stars from '$lib/level/stars/store'
	import LevelScene from '../../components/Level/Scene.svelte'

	export let id: number

	$: enabled = $stars && id - 1 <= $stars.length
	$: if (enabled === false) goto('/levels').catch(console.error)
</script>

{#if enabled}
	<LevelScene {id} />
{/if}
