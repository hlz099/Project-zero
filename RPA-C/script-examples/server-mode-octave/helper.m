rpa_url = "http://localhost:8080";
global evaluate_url = [rpa_url "/evaluate"];
global reset_url = [rpa_url "/reset"];

function result = rpa_execute(script)
    global evaluate_url;
    result = urlread(evaluate_url, "post", {"script", script});
endfunction

function rpa_reset()
    global reset_url;
    urlread(reset_url, "post", "");
endfunction

function script = rpa_load_script(fname)
    script = fileread(fname);
endfunction

