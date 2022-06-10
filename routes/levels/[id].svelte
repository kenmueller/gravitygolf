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

	import levels from '$lib/level/levels.json'
	import BackLink from '../../components/Link/Back.svelte'

	export let id: number
</script>

<svelte:head>
	<title>Level {id}</title>
</svelte:head>

<BackLink href="/levels" />

<h1>Level {id}</h1>
