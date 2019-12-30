# rofi-blocks

Rofi modi that allows controlling rofi content throug communication with an external program

To run this you need an up to date checkout of rofi git installed.

Run rofi like:

```bash
rofi -modi blocks -show blocks
```
or
```bash
rofi -modi blocks -show blocks -blocks-wrap /path/to/program
```

### Dependencies

| Dependency | Version |
|------------|---------|
| rofi 	     | 1.4     |
| json-glib  | 1.0     |


### Installation

**Rofi-blocks** uses autotools as build system. When installing from git, the following steps should install it:

```bash
$ autoreconf -i
$ mkdir build
$ cd build/
$ ../configure
$ make
$ make install
```


### Background

This module was created to extend rofi scripting capabilities, such as:

 - Changing the message and list content on the fly

 - Allow update of content on intervals, without the need of interaction (e.g. top)

 - Allow changing the input prompt on the fly, or be defined on the script instead of rofi command line

 - Allow custom overlay message
 

### Documentation

The module works by reading the stdin line by line to update rofi content. It can execute an application in the background, which will be used to communicate with the modi. The communication is made using file descriptors. The modi reads the output of the backgroung program and writes events its input.

#### Communication

This modi updates rofi each time it reads a line from the stdin or background program output, and writes an event made from rofi, in most cases is an interaction with the window, as a single line to stdout or its input. The format used for communication is JSON format.

#### Why JSON:

For the following reasons:
1. All information can be stored in one line
2. It is simple to write

##### All information can be stored in one line

 New lines on message and text can be escaped with `\n` , this helps the modi to know when to stop reading and starts parsing the message, preventing cases of the program flushing incomplete lines, and json parser failing due to incomplete messages.

##### It is simple to write
There is no need to have a library or framework to write json, there are, however, few consideration when transforming text to json string, such as escaping backlashes, newlines and double quotes, something like that can be done with a string replacer, like the `replace()` method in python or `sed` command in bash


#### Output JSON format

The JSON format used to communicate with the modi is the one on the next figure, all fields are optional:

```json
{
  "message": "rofi message text",
  "overlay": "overlay text",
  "prompt": "prompt text",
  "input": "input text",
  "input action": "input action, 'send' or 'filter'",
  "event format": "event format",
  "lines":[
    "one line is a line from input", 
    {"text":"can be a string or object"}, 
    {"text":"with object, you can use flags", "urgent":true},
    {"text":"such as urgent and highlight", "highlight": true},
    {"text":"or both", "urgent":true, "highlight": true}
  ]
}
```

##### Property table

| Property     | Description                                          |
|--------------|------------------------------------------------------|
| message 	   | Sets rofi message                                    |
| overlay      | Shows overlay with text, hides it if empty or null   |
| prompt       | sets prompt text                                     |
| input action | Sets input change action only two values are accepted, any other is ignored. <br> **filter***(default)*: rofi filters the content, no event is sent to program input <br> **send**: prevents rofi filter and sends an "*input change*" event to program input |
| event format | event format used to send to input, more of it on next section |
| lines        | a list of sting or json object to set rofi list content, a string will show a text with all flags disabled.  |

#### Input format

The default format that the plugin uses to write events to the program is a json object. However, there are scripting languages that does not have a builtin json parser, *shell* is one of them. For that reason, the event format is configurable by setting the "*event format*" property on output. 

The default format has the following text: 
```json
{"name":"{{name_escaped}}", "value":"{{value_escaped}}"}
```

It contains some text wrapped with double braces {{}}. These wrapped text indicates a parameter that will be replaced by the plugin formatter. The following parameters are supported:

| Parameter       | Format              | Description                                                               |
|-----------------|---------------------|---------------------------------------------------------------------------|
| name            | `{{name}}`          | name of the event                                                         |
| name escaped    | `{{name_escaped}}`  | name of the event, escaped to be inserted on a json string                |
| name as enum    | `{{name_enum}}`     | name of the event in all caps, separated by underscore for easier parsing |
| value           | `{{value}}`         | information of the event: <br> - **line content** on entry select or delete <br> - **input content** on input change or exec custom input <br> - **custom key number** on custom key typed |
| value escaped   | `{{value_escaped}}` | information of the event, escaped to be inserted on a json string         |

##### Event names:

| Name                   | Name as enumerator | Description                                               |
|------------------------|--------------------|-----------------------------------------------------------|
| input change           | INPUT_CHANGE       | sent when input changes and input action is set to `send` |
| custom key             | CUSTOM_KEY         | sent when a custom key is typed                           |
| select entry           | SELECT_ENTRY       | sent when selecting an entry on the list                  |
| delete entry           | DELETE_ENTRY       | sent when deleting an entry on the list                   |
| execute custom input   | EXEC_CUSTOM_INPUT  | sent when a custom input is to be executed                |


### Examples

Additional documentation is created in the form of functional examples, you can compile and install the modi and execute examples in examples folder to understand and play with the modi.
