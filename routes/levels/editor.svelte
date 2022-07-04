<script lang="ts">
	import mobile from '$lib/mobile'
	import landscape from '$lib/landscape'
	import MetaImage from '../../components/Meta/Image.svelte'
	import MetaTitle from '../../components/Meta/Title.svelte'
	import MetaDescription from '../../components/Meta/Description.svelte'
	import LevelEditor from '../../components/Level/Editor.svelte'
	import BackLink from '../../components/Link/Back.svelte'
</script>

<MetaImage />
<MetaTitle value="Level Editor | Gravity Golf" />
<MetaDescription />

{#if !$mobile || ($mobile && $landscape)}
	<LevelEditor />
{:else}
	<main>
		<BackLink href="/">Level Editor</BackLink>
		<h1>Rotate your device to edit</h1>
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
