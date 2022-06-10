<script lang="ts">
	import Play from '../images/Play.svelte'
</script>

<svelte:head>
	<title>Gravity Golf</title>
</svelte:head>

<main>
	<h1>Gravity Golf</h1>
	<a href="/levels">
		<Play />
	</a>
</main>

<style lang="scss">
	main {
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		height: 100%;
	}

	h1 {
		font-size: 3rem;
		color: white;
	}

	a {
		margin-top: 4rem;
		padding: 1.5rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			display: block;
			width: 4rem;
			transform: translateX(0.1rem);
		}
	}
</style>
