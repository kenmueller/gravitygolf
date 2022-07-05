<script context="module" lang="ts">
	export const load: Load = async ({ fetch, params: { id } }) => {
		try {
			const level = await getCommunityLevel(fetch, id)

			if (!level)
				return {
					status: ErrorCode.TemporaryRedirect,
					redirect: '/levels/community'
				}

			return {
				props: { level }
			}
		} catch (value) {
			const { code, message } = errorFromValue(value)
			return { status: code, error: message }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import type CommunityLevel from '$lib/level/community'
	import getCommunityLevel from '$lib/level/community/get'
	import ErrorCode from '$lib/error/code'
	import errorFromValue from '$lib/error/from/value'
	import mobile from '$lib/mobile'
	import landscape from '$lib/landscape'
	import MetaImage from '../../../components/Meta/Image.svelte'
	import MetaTitle from '../../../components/Meta/Title.svelte'
	import MetaDescription from '../../../components/Meta/Description.svelte'
	import LevelScene from '../../../components/Level/Scene.svelte'
	import BackLink from '../../../components/Link/Back.svelte'

	export let level: CommunityLevel
</script>

<MetaImage />
<MetaTitle value="{level.name} | Gravity Golf" />
<MetaDescription />

{#if !$mobile || ($mobile && $landscape)}
	<!-- <LevelScene {level} /> -->
{:else}
	<main>
		<BackLink href="/levels/community">{level.name}</BackLink>
		<h1>Rotate your device to play</h1>
	</main>
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
