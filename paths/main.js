
function run() {
    let p = document.getElementById("test");
    p.innerHTML = "Hi there! This is Mr.JavaScript speaking";
    p.innerText = "Hi there! This is Mr.JavaScript speaking";
    console.log("Hello Browser! This is server speaking");

}

window.onload = run;
