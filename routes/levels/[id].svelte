<script context="module" lang="ts">
	export const load: Load = ({ params }) => {
		const id = Number.parseInt(params.id)

		if (Number.isNaN(id))
			return { status: ErrorCode.PermanentRedirect, redirect: '/levels' }

		if (id - 1 < 0)
			return { status: ErrorCode.PermanentRedirect, redirect: '/levels' }

		if (id - 1 >= levels.length)
			return { status: ErrorCode.TemporaryRedirect, redirect: '/levels' }

		return {
			props: { id }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import { goto } from '$app/navigation'

	import type RawLevel from '$lib/level/raw'
	import levelFromRaw from '$lib/level/raw/from'
	import ErrorCode from '$lib/error/code'
	import levels from '$lib/level/levels'
	import stars from '$lib/level/stars'
	import mobile from '$lib/mobile'
	import landscape from '$lib/landscape'
	import setStars from '$lib/level/stars/set'
	import MetaImage from '../../components/Meta/Image.svelte'
	import MetaTitle from '../../components/Meta/Title.svelte'
	import MetaDescription from '../../components/Meta/Description.svelte'
	import LevelScene from '../../components/Level/Scene.svelte'
	import BackLink from '../../components/Link/Back.svelte'

	export let id: number

	$: enabled = $stars && id - 1 <= $stars.length
	$: if (enabled === false) goto('/levels').catch(console.error)

	$: name = `Level ${id}`
	$: level = levelFromRaw(levels[id - 1] as RawLevel)

	$: hasNext = id !== levels.length
</script>

<MetaImage />
<MetaTitle value="{name} | Gravity Golf" />
<MetaDescription />

{#if enabled}
	{#if !$mobile || $landscape}
		<LevelScene
			{name}
			{level}
			setStars={stars => setStars(id, stars)}
			back="/levels"
			{hasNext}
			next="/levels{hasNext ? `/${id + 1}` : ''}"
		/>
	{:else}
		<main>
			<BackLink href="/levels">{name}</BackLink>
			<h1>Rotate your device to play</h1>
		</main>
	{/if}
{/if}

<style lang="scss">
	main {
		position: relative;
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		overflow: hidden;
		height: 100%;
		padding: 2rem;

		> :global(a) {
			position: absolute;
			top: 1.5rem;
			left: 1.7rem;
		}
	}

	h1 {
		text-align: center;
		color: white;
	}
</style>
