<script>
import Character from "./Character.svelte";

export let config;
export let num_selected = 0;

function checkbox_onchange(e) {
    if (e.target.checked) {
        num_selected += 1;
    }
    else {
        num_selected -= 1;
    }
}

</script>

<h4>Select characters</h4>
<div class="characters">
  {#each config.characters as c}
  <Character character={c} onchange={checkbox_onchange}/>
  {/each}
</div>

<style>
.characters {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  margin-bottom: 2em;
}
</style>