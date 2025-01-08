rpa_url = "http://localhost:8080";

// Load script as a text from specified path
function [script]=rpa_load_script(file_name)
    details = fileinfo(file_name);
    len = details(1);
    fd = mopen(file_name, 'rt');
    script = mgetstr(len, fd);
    mclose(fd);
endfunction

// Reset the server context
function []=rpa_reset()
    [res,code] = http_post(rpa_url+"/reset");
endfunction

// Execute the script passed as a text
// Returned values:
//    res - result; depends on the script
//    code - HTTP status code; has to be 200 on success
function [res,code]=rpa_execute(script)
    [res,code] = http_post(rpa_url+"/evaluate", script);
endfunction
