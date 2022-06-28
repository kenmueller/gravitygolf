<script lang="ts">
	import levels from '$lib/level/levels'
	import mobile from '$lib/mobile'
	import MetaImage from '../../components/Meta/Image.svelte'
	import MetaTitle from '../../components/Meta/Title.svelte'
	import MetaDescription from '../../components/Meta/Description.svelte'
	import Level from '../../components/Levels/Level.svelte'
	import BackLink from '../../components/Link/Back.svelte'
</script>

<MetaImage />
<MetaTitle value="Levels | Gravity Golf" />
<MetaDescription />

<main>
	<header>
		<BackLink href="/">
			Levels
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
	</header>
	<div>
		{#each levels as _level, index}
			<Level id={index + 1} />
		{/each}
	</div>
</main>

<style lang="scss">
	main {
		display: flex;
		flex-direction: column;
		align-items: center;
		height: 100%;
		overflow-y: auto;
		padding: 2rem 0;
	}

	header,
	div {
		max-width: 62.5rem;
		width: 95%;
	}

	header {
		display: flex;
		align-items: center;
	}

	div {
		display: grid;
		grid-auto-rows: 5rem;
		grid-template-columns: repeat(auto-fit, minmax(15rem, 1fr));
		gap: 1.5rem;
		margin-top: 1.5rem;

		@media (min-width: 58rem) {
			gap: 2rem;
		}
	}
</style>
