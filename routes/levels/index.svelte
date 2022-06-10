<script lang="ts">
	import levels from '$lib/level/levels.json'
	import Level from '../../components/Levels/Level.svelte'
	import Back from '../../images/Back.svelte'
</script>

<svelte:head>
	<title>Levels | Gravity Golf</title>
</svelte:head>

<main>
	<header>
		<a href="/">
			<Back />
			Gravity Golf
		</a>
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

	a {
		display: flex;
		align-items: center;
		text-decoration: none;
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			display: block;
			width: 2.5rem;
		}
	}

	div {
		display: grid;
		grid-auto-rows: 5rem;
		grid-template-columns: repeat(auto-fit, minmax(15rem, 1fr));
		gap: 2rem;
		margin-top: 1.5rem;
	}
</style>
