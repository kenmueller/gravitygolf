<script lang="ts">
	import MetaImage from '../components/Meta/Image.svelte'
	import MetaTitle from '../components/Meta/Title.svelte'
	import MetaDescription from '../components/Meta/Description.svelte'
	import Play from '../images/Play.svelte'
</script>

<MetaImage />
<MetaTitle />
<MetaDescription />

<main>
	<h1>Gravity Golf</h1>
	<a href="/levels" aria-label="Levels">
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
			width: 4rem;
			transform: translateX(0.1rem);
		}
	}
</style>
