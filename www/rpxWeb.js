// Require in some of the native stuff that comes with Node
const http = require('http');
const path = require('path');
const fs = require('fs');


// Port number to use
const port = 80;
// Colors for CLI output
const WHITE = '\033[39m';
const RED = '\033[91m';
const GREEN = '\033[32m';

// Setting up MIME-Type (YOU MAY NEED TO ADD MORE HERE) <--------
const contentTypesByExtension = {
    '.html': 'text/html',
    '.css':  'text/css',
    '.js':   'text/javascript',
    '.json': 'text/json',
    '.svg':  'image/svg+xml'
};

/**
 * Resolve, which file the server should send to a client.
 * It could be a requested file when the file exists or the index file on a directory.
 * If the file does not exist, we return the path to the 404 page.
 * @param stats stat result of the file name
 * @param filename the name of the requested file
 * @returns {string} absolute file name which should be served by the server
 */
function resolveFilenameToSend(stats, filename) {
    if (stats == null) {
        // Output a red error pointing to failed request
        console.log(RED + 'FAIL: ' + filename);
        // Redirect the browser to the 404 page
        filename = path.join(process.cwd(), '/404.html');
    } else if (stats.isDirectory()) {
        // Output a green line to the console explaining what folder was requested
        console.log(GREEN + 'FLDR: ' + WHITE + filename);
        // redirect the user to the index.html in the requested folder
        filename += '/rpx100.html';
    } else if (!stats.isFile()) {
        // Output a red error pointing to failed request
        console.log(RED + 'FAIL: ' + filename);
        // Redirect the browser to the 404 page
        filename = path.join(process.cwd(), '/404.html');
    }
    return filename;
}

/**
 * Send an internal server error response to the client.
 *
 * @param response the response object.
 * @param err the error message
 */
function sendErrorResponse(response, err) {
    response.writeHead(500, {'Content-Type': 'text/plain'});
    response.write(err + '\n');
    response.end();
}

/**
 * Determine the content type of the file based on the file suffix and set it in the headers object.
 *
 * For example, passing in a file name of "test.css" will result in the 'Content-Type' being set to "text/css"
 *
 * @param filename the filename from which the suffix will be used
 * @param headers the headers object to update
 */
function setContentTypeForSuffix(filename, headers) {
    const contentType = contentTypesByExtension[path.extname(filename)];
    // If the requested file has a matching MIME-Type
    if (contentType) {
        // Set it in the headers
        headers['Content-Type'] = contentType;
    }
}

// Create the server
http.createServer(function (request, response) {

    // get the /file.html from above and then find it from the current folder
    let filename = path.join(process.cwd(), request.url);

    // Check if the requested file exists
    fs.stat(filename, function (err, stats) {
        // If it doesn't
        filename = resolveFilenameToSend(stats, filename);

        // Assuming the file exists, read it
        fs.readFile(filename, 'binary', function (err, file) {
            // Output a green line to console explaining the file that will be loaded in the browser
            console.log(GREEN + 'FILE: ' + WHITE + filename);
            // If there was an error trying to read the file
            if (err) {
                // Put the error in the browser
                sendErrorResponse(response, err);
                return;
            }

            // Otherwise, declare a headers object and a var for the MIME-Type
            const headers = {};
            setContentTypeForSuffix(filename, headers);

            // Output the read file to the browser for it to load
            response.writeHead(200, headers);
            response.write(file, 'binary');
            response.end();
        });

    });

}).listen(port);

// Nachricht, die im Terminal ausgegeben wird.
console.log(WHITE + 'Static file server running at\n  => http://localhost:' + port + '/\nCTRL + C to shutdown');
