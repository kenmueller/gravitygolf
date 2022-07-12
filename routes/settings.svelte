<script lang="ts">
	import mobile from '$lib/mobile'
	import deleteLevelProgress from '$lib/level/progress/remove'
	import MetaImage from '../components/Meta/Image.svelte'
	import MetaTitle from '../components/Meta/Title.svelte'
	import MetaDescription from '../components/Meta/Description.svelte'
	import BackLink from '../components/Link/Back.svelte'
	import Trash from '../images/Trash.svelte'

	const deleteProgress = () => {
		switch (prompt("Type 'delete' to delete all progress")) {
			case 'delete':
				deleteLevelProgress()
				alert('All progress was deleted')
				break
			case null:
				break
			default:
				alert("'delete' was typed incorrectly")
		}
	}
</script>

<MetaImage />
<MetaTitle value="Settings | Gravity Golf" />
<MetaDescription />

<main>
	<header>
		<BackLink href="/">
			Settings
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
	</header>
	<div>
		<button class="delete-progress" on:click={deleteProgress}>
			<Trash />
			<span>Delete all progress</span>
		</button>
	</div>
</main>

<style lang="scss">
	main {
		display: flex;
		flex-direction: column;
		align-items: center;
		overflow: hidden;
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
		justify-content: space-between;
		align-items: center;
	}

	div {
		display: flex;
		flex-direction: column;
		align-items: center;
		margin-top: 1.5rem;
	}

	.delete-progress {
		display: flex;
		align-items: center;
		max-width: 20rem;
		width: 100%;
		padding: 0.5rem 0.5rem;
		font-size: 1rem;
		color: white;
		background: rgb(222, 75, 70);
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		&:hover {
			opacity: 0.7;
		}

		> :global(svg) {
			width: 2rem;
		}

		> span {
			margin: 0 auto;
		}
	}
</style>
